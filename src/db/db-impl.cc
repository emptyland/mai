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
#include "mai/env.h"
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
        default_cf_.reset(new ColumnFamilyHandle(this, versions_->column_families()->GetDefault()));
    }
    return rs;
}
    
Error DBImpl::NewDB(const Options &opts,
                    const std::vector<ColumnFamilyDescriptor> &desc) {
    table_cache_.reset(new TableCache(db_name_, opts.env, factory_.get(),
                                      opts.allow_mmap_reads));
    versions_.reset(new VersionSet(db_name_, opts));
    
    
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
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error DBImpl::Delete(const WriteOptions &opts, ColumnFamily *cf,
                     std::string_view key) {
    return MAI_NOT_SUPPORTED("TODO:");
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
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Iterator *
DBImpl::NewIterator(const ReadOptions &opts, ColumnFamily *cf) {
    return nullptr;
}
    
/*virtual*/ const Snapshot *DBImpl::GetSnapshot() {
    return nullptr;
}
    
/*virtual*/ void DBImpl::ReleaseSnapshot(const Snapshot *snapshot) {
}
    
/*virtual*/ ColumnFamily *DBImpl::DefaultColumnFamily() {
    return default_cf_.get();
}
    
Error DBImpl::MakeRoomForWrite(ColumnFamilyImpl *cf, bool force) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
} // namespace db    
    
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
