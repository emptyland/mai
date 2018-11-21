#include "db/db-impl.h"
#include "db/write-ahead-log.h"
#include "db/version.h"
#include "db/column-family.h"
#include "db/table-cache.h"
#include "db/files.h"
#include "db/factory.h"
#include "db/table-cache.h"
#include "core/key-boundle.h"
#include "core/memory-table.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
    
namespace db {
    
class WritingHandler final : public WriteBatch::Stub {
public:
    WritingHandler(core::SequenceNumber sequence_number,
                   ColumnFamilySet *column_families)
        : last_sequence_number_(sequence_number)
        , column_families_(DCHECK_NOTNULL(column_families)) {}
    
    virtual ~WritingHandler() {}
    
    virtual void Put(uint32_t cfid, std::string_view key,
                     std::string_view value) override {
        ColumnFamilyImpl *impl = EnsureGetColumnFamily(cfid);
        impl->mutable_table()->Put(key, value, sequence_number(),
                                   core::Tag::kFlagValue);
        
        size_count_ += key.size() + sizeof(uint32_t) + sizeof(uint64_t);
        size_count_ += value.size();
    }
    
    virtual void Delete(uint32_t cfid, std::string_view key) override {
        ColumnFamilyImpl *impl = EnsureGetColumnFamily(cfid);
        impl->mutable_table()->Put(key, "", sequence_number(),
                                   core::Tag::kFlagDeletion);
        size_count_ += key.size() + sizeof(uint32_t) + sizeof(uint64_t);
    }
    
    core::SequenceNumber sequence_number() const {
        return last_sequence_number_ + sequence_number_count_;
    }
    
    ColumnFamilyImpl *EnsureGetColumnFamily(uint32_t cfid) {
        ColumnFamilyImpl *impl = cfid == 0 ? column_families_->GetDefault() :
            column_families_->GetColumnFamily(cfid);
        DCHECK_NOTNULL(impl);
        DCHECK(impl->initialized());
        return impl;
    }
    
    DEF_VAL_GETTER(uint64_t, size_count);
    DEF_VAL_GETTER(uint64_t, sequence_number_count);
private:
    core::SequenceNumber last_sequence_number_;
    ColumnFamilySet *column_families_;
    
    uint64_t size_count_ = 0;
    uint64_t sequence_number_count_ = 0;
}; // class WritingHnalder
    
    
    
struct GetContext {
    std::vector<base::Handle<core::MemoryTable>> in_mem;
    core::SequenceNumber last_sequence_number;
    ColumnFamilyImpl *cfd;
}; // struct ReadContext

    

DBImpl::DBImpl(const std::string &db_name, Env *env)
    : db_name_(db_name)
    , env_(DCHECK_NOTNULL(env))
    , factory_(Factory::Default())
{
}

DBImpl::~DBImpl() {
    
}
    
Error DBImpl::Open(const Options &opts,
                   const std::vector<ColumnFamilyDescriptor> &desc,
                   std::vector<ColumnFamily *> *column_families) {
    Error rs;
    //shutting_down_.store(nullptr, std::memory_order_release);

    rs = env_->FileExists(Files::CurrentFileName(db_name_));
    if (rs.fail()) { // Not exists
        if (!opts.create_if_missing) {
            return MAI_CORRUPTION("db miss and create_if_missing is false.");
        }
        rs = NewDB(opts, desc);
    } else {
        if (opts.error_if_exists) {
            return MAI_CORRUPTION("db exists and error_if_exists is true.");
        }
        rs = Recovery(opts, desc);
    }
    if (rs.ok()) {
        for (const auto &d : desc) {
            ColumnFamilyImpl *impl =
                versions_->column_families()->GetColumnFamily(d.name);
            DCHECK_NOTNULL(impl);
            column_families->push_back(new ColumnFamilyHandle(this, impl));
        }
        ColumnFamily *handle =
            new ColumnFamilyHandle(this,
                                   versions_->column_families()->GetDefault());
        default_cf_.reset(handle);
    }
    return rs;
}
    
Error DBImpl::NewDB(const Options &opts,
                    const std::vector<ColumnFamilyDescriptor> &desc) {
    table_cache_.reset(new TableCache(db_name_, opts.env, factory_.get(),
                                      opts.allow_mmap_reads));
    versions_.reset(new VersionSet(db_name_, opts, table_cache_.get()));
    
    
    Error rs = env_->MakeDirectory(db_name_, true);
    if (!rs) {
        return rs;
    }
    log_file_number_ = versions_->GenerateFileNumber();
    std::string log_file_name = Files::LogFileName(db_name_, log_file_number_);
    rs = env_->NewWritableFile(log_file_name, &log_file_);
    if (!rs) {
        return rs;
    }
    logger_.reset(new LogWriter(log_file_.get(), WAL::kDefaultBlockSize));

    std::map<std::string, ColumnFamilyOptions> cf_opts;
    for (const auto &d : desc) {
        cf_opts[d.name] = d.options;
    }

    // No default column family
    auto iter = cf_opts.find(kDefaultColumnFamilyName);
    if (iter == cf_opts.end()) {
        versions_->column_families()->NewColumnFamily(opts,
                                                      kDefaultColumnFamilyName,
                                                      0, versions_.get());
    } else {
        versions_->column_families()->NewColumnFamily(iter->second,
                                                      kDefaultColumnFamilyName,
                                                      0, versions_.get());
    }
    for (const auto &opt : cf_opts) {
        if (opt.first.compare(kDefaultColumnFamilyName) == 0) {
            continue;
        }
        uint32_t cfid = versions_->column_families()->NextColumnFamilyId();
        versions_->column_families()->NewColumnFamily(opt.second,
                                                      kDefaultColumnFamilyName,
                                                      cfid, versions_.get());
    }
    for (ColumnFamilyImpl *impl : *versions_->column_families()) {
        rs = impl->Install(factory_.get(), env_);
        if (!rs) {
            return rs;
        }
    }
    VersionPatch patch;
    patch.set_prev_log_number(0);
    patch.set_redo_log_number(log_file_number_);
    return versions_->LogAndApply(opts, &patch, &mutex_);
}

Error DBImpl::Recovery(const Options &opts,
                       const std::vector<ColumnFamilyDescriptor> &desc) {
    return MAI_NOT_SUPPORTED("TODO:");
}

/*virtual*/ Error
DBImpl::NewColumnFamilies(const std::vector<std::string> &names,
                  const ColumnFamilyOptions &options,
                  std::vector<ColumnFamily *> *result) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error
DBImpl::DropColumnFamilies(const std::vector<ColumnFamily *> &column_families) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error
DBImpl::GetAllColumnFamilies(std::vector<ColumnFamily *> *result) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error DBImpl::Put(const WriteOptions &opts, ColumnFamily *cf,
                  std::string_view key, std::string_view value) {
    return Write(opts, cf, key, value, core::Tag::kFlagValue);
}
    
/*virtual*/ Error DBImpl::Delete(const WriteOptions &opts, ColumnFamily *cf,
                     std::string_view key) {
    return Write(opts, cf, key, "", core::Tag::kFlagDeletion);
}
    
/*virtual*/ Error DBImpl::Write(const WriteOptions& opts, WriteBatch* updates) {
    core::SequenceNumber last_version = 0;
    Error rs;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        last_version = versions_->last_sequence_number();
        
        rs = MakeRoomForWrite(nullptr, false);
        if (!rs) {
            return rs;
        }
    }
    rs = logger_->Append(updates->redo());
    if (!rs) {
        return rs;
    }
    
    if (opts.sync) {
        rs = log_file_->Sync();
        if (!rs) {
            return rs;
        }
    }
    
    WritingHandler handler(last_version + 1, versions_->column_families());
    {
        base::ReaderSpinLock lock(versions_->column_families()->rw_mutex());
        updates->Iterate(&handler);
    }
    
    mutex_.lock();
    versions_->AddSequenceNumber(handler.sequence_number_count());
    mutex_.unlock();
    return Error::OK();
}
    
/*virtual*/ Error DBImpl::Get(const ReadOptions &opts, ColumnFamily *cf,
                  std::string_view key, std::string *value) {
    
    using core::Tag;
    
    GetContext ctx;
    Error rs = PrepareForGet(opts, cf, &ctx);
    for (const auto &table : ctx.in_mem) {
        rs = table->Get(key, ctx.last_sequence_number, nullptr, value);
        if (rs.ok()) {
            return rs;
        }
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    return ctx.cfd->current()->Get(opts, key, ctx.last_sequence_number, nullptr,
                                   value);
}
    
/*virtual*/ Iterator *
DBImpl::NewIterator(const ReadOptions &opts, ColumnFamily *cf) {
    GetContext ctx;
    Error rs = PrepareForGet(opts, cf, &ctx);
    if (!rs) {
        return Iterator::AsError(rs);
    }
    
    std::vector<Iterator *> iters;
    for (const auto &table : ctx.in_mem) {
        iters.push_back(table->NewIterator());
    }
    rs = ctx.cfd->AddIterators(opts, &iters);
    if (!rs) {
        for (auto iter : iters) {
            delete iter;
        }
        return Iterator::AsError(rs);
    }

    // TODO:
    return Iterator::AsError(MAI_NOT_SUPPORTED("TODO:"));
}
    
/*virtual*/ const Snapshot *DBImpl::GetSnapshot() {
    return nullptr;
}
    
/*virtual*/ void DBImpl::ReleaseSnapshot(const Snapshot *snapshot) {
}
    
/*virtual*/ ColumnFamily *DBImpl::DefaultColumnFamily() {
    return default_cf_.get();
}
    
Error DBImpl::PrepareForGet(const ReadOptions &opts, ColumnFamily *cf,
                            GetContext *ctx) {
    if (cf == nullptr) {
        return MAI_CORRUPTION("NULL column family.");
    }
    ColumnFamilyHandle *handle = static_cast<ColumnFamilyHandle *>(cf);
    if (this != handle->db()) {
        return MAI_CORRUPTION("Use difference db column family.");
    }
    ColumnFamilyImpl *cfd = handle->impl();
    if (cfd->IsDropped()) {
        return MAI_CORRUPTION("Column family has been dropped.");
    }
    
    core::SequenceNumber last_sequence_number = 0;
    mutex_.lock(); // Locking versions------------------------------------------
    if (opts.snapshot) {
        last_sequence_number =
            static_cast<const SnapshotImpl *>(opts.snapshot)->sequence_number();
    } else {
        last_sequence_number = versions_->last_sequence_number();
    }
    
    std::vector<base::Handle<core::MemoryTable>> mem_tables;
    mem_tables.push_back(base::MakeRef(cfd->mutable_table()));
    if (cfd->immutable_table()) {
        mem_tables.push_back(base::MakeRef(cfd->immutable_table()));
    }
    mutex_.unlock(); // Unlocking versions--------------------------------------
    
    return Error::OK();
}
    
Error DBImpl::Write(const WriteOptions &opts, ColumnFamily *cf,
                    std::string_view key, std::string_view value, uint8_t flag) {
    using core::Tag;
    
    ColumnFamilyHandle *handle = static_cast<ColumnFamilyHandle *>(cf);
    if (this != handle->db()) {
        return MAI_CORRUPTION("Use difference db column family.");
    }
    ColumnFamilyImpl *cfd = handle->impl();
    if (cfd->IsDropped()) {
        return MAI_CORRUPTION("Column family has been dropped.");
    }
    
    core::SequenceNumber last_sequence_number = 0;
    Error rs;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        last_sequence_number = versions_->last_sequence_number();
        
        rs = MakeRoomForWrite(cfd, false);
        if (!rs) {
            return rs;
        }
    }
    
    std::string redo;
    core::KeyBoundle::MakeRedo(key, value, cfd->id(), flag, &redo);
    rs = logger_->Append(redo);
    if (!rs) {
        return rs;
    }
    if (opts.sync) {
        rs = log_file_->Sync();
        if (!rs) {
            return rs;
        }
    }
    
    cfd->mutable_table()->Put(key, value, last_sequence_number, flag);
    
    mutex_.lock();
    versions_->AddSequenceNumber(1);
    mutex_.unlock();
    
    return Error::OK();
}
    
Error DBImpl::MakeRoomForWrite(ColumnFamilyImpl *cf, bool force) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
} // namespace db
    
const char kDefaultColumnFamilyName[] = "default";
    
/*static*/ Error DB::Open(const Options &opts,
                          const std::string name,
                          const std::vector<ColumnFamilyDescriptor> &descriptors,
                          std::vector<ColumnFamily *> *column_families,
                          DB **result) {
    db::DBImpl *impl = new db::DBImpl(name, opts.env);
    Error rs = impl->Open(opts, descriptors, column_families);
    if (!rs) {
        return rs;
    }
    
    *result = impl;
    return Error::OK();
}
    
} // namespace mai
