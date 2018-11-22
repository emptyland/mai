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
                   ColumnFamilySet *column_families,
                   std::mutex *db_mutex)
        : last_sequence_number_(sequence_number)
        , column_families_(DCHECK_NOTNULL(column_families))
        , db_mutex_(DCHECK_NOTNULL(db_mutex)) {}
    
    virtual ~WritingHandler() {}
    
    virtual void Put(uint32_t cfid, std::string_view key,
                     std::string_view value) override {
        base::Handle<core::MemoryTable> table;
        EnsureGetTable(cfid, &table);

        table->Put(key, value, sequence_number(), core::Tag::kFlagValue);
        
        size_count_ += key.size() + sizeof(uint32_t) + sizeof(uint64_t);
        size_count_ += value.size();
        sequence_number_count_ ++;
    }
    
    virtual void Delete(uint32_t cfid, std::string_view key) override {
        base::Handle<core::MemoryTable> table;
        EnsureGetTable(cfid, &table);

        table->Put(key, "", sequence_number(), core::Tag::kFlagDeletion);
        size_count_ += key.size() + sizeof(uint32_t) + sizeof(uint64_t);
        sequence_number_count_ ++;
    }
    
    core::SequenceNumber sequence_number() const {
        return last_sequence_number_ + sequence_number_count_;
    }
    
    void EnsureGetTable(uint32_t cfid, base::Handle<core::MemoryTable> *table) {
        ColumnFamilyImpl *impl = (cfid == 0) ? column_families_->GetDefault() :
            EnsureGetColumnFamily(cfid);
        *table = impl->mutable_table();
    }
    
    ColumnFamilyImpl *EnsureGetColumnFamily(uint32_t cfid) {
        // Locking DB ----------------------------------------------------------
        std::unique_lock<std::mutex> lock(*db_mutex_);
        ColumnFamilyImpl *impl = (cfid == 0) ? column_families_->GetDefault() :
            column_families_->GetColumnFamily(cfid);
        DCHECK_NOTNULL(impl);
        DCHECK(impl->initialized());
        return impl;
        // Unlocking DB --------------------------------------------------------
    }
    
    DEF_VAL_GETTER(uint64_t, size_count);
    DEF_VAL_GETTER(uint64_t, sequence_number_count);
private:
    const core::SequenceNumber last_sequence_number_;
    ColumnFamilySet *const column_families_;
    std::mutex *const db_mutex_;
    
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
    , abs_db_path_(env->GetAbsolutePath(db_name))
    , env_(DCHECK_NOTNULL(env))
    , factory_(Factory::NewDefault()) {
}

DBImpl::~DBImpl() {
    default_cf_.reset(); // Release default column family handle first.
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
        DCHECK_NOTNULL(column_families)->clear();
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
    rs = env_->NewWritableFile(log_file_name, true, &log_file_);
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
    table_cache_.reset(new TableCache(db_name_, opts.env, factory_.get(),
                                      opts.allow_mmap_reads));
    versions_.reset(new VersionSet(db_name_, opts, table_cache_.get()));
    
    
    std::string current_file_name = Files::CurrentFileName(db_name_);
    std::string result;
    Error rs = base::FileReader::ReadAll(current_file_name, &result, env_);
    if (!rs) {
        return rs;
    }
    if (result.empty()) {
        return MAI_CORRUPTION("manifest_file_number is not a number!");
    }
    for (char c : result) {
        if (!::isdigit(c)) {
            return MAI_CORRUPTION("manifest_file_number is not a number!");
        }
    }
    
    uint64_t manifest_file_number = ::atoll(result.c_str());
    std::vector<uint64_t> logs;
    std::map<std::string, ColumnFamilyOptions> cf_opts;
    for (const auto &d : desc) {
        cf_opts[d.name] = d.options;
    }
    rs = versions_->Recovery(cf_opts, manifest_file_number, &logs);
    if (!rs) {
        return rs;
    }
    DCHECK_GE(logs.size(), 2);

    for (ColumnFamilyImpl *impl : *versions_->column_families()) {
        cf_opts.erase(impl->name());
    }
    for (const auto &remain : cf_opts) {
        uint32_t cfid;
        rs = InternalNewColumnFamily(remain.first, remain.second, &cfid);
        if (!rs) {
            return rs;
        }
    }
    for (ColumnFamilyImpl *impl : *versions_->column_families()) {
        if (!impl->initialized()) {
            rs = impl->Install(factory_.get(), env_);
            if (!rs) {
                return rs;
            }
        }
        DLOG(INFO) << "Column family: " << impl->name() << " installed.";
    }
    
    rs = Redo(versions_->redo_log_number(), logs[logs.size() - 2]);
    if (!rs) {
        return rs;
    }
    DLOG(INFO) << "Replay ok, last version: "
               << versions_->last_sequence_number();
    
    log_file_number_ = versions_->redo_log_number();
    rs = env_->NewWritableFile(Files::LogFileName(db_name_, log_file_number_),
                               true, &log_file_);
    if (!rs) {
        return rs;
    }
    logger_.reset(new LogWriter(log_file_.get(), WAL::kDefaultBlockSize));
    
    return Error::OK();
}

/*virtual*/ Error
DBImpl::NewColumnFamilies(const std::vector<std::string> &names,
                  const ColumnFamilyOptions &options,
                  std::vector<ColumnFamily *> *result) {
    DCHECK_NOTNULL(result)->clear();
    
    Error rs;
    VersionPatch patch;
    std::set<uint32_t> succ;
    
    // Locking versions---------------------------------------------------------
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto &name : names) {
        uint32_t cfid;
        rs = InternalNewColumnFamily(name, options, &cfid);
        if (!rs) {
            break;
        }
        succ.insert(cfid);
    }
    for (uint32_t cfid : succ) {
        ColumnFamilyImpl *cfd =
            versions_->column_families()->GetColumnFamily(cfid);
        Error install_rs = cfd->Install(factory_.get(), env_);
        if (install_rs.ok()) {
            ColumnFamily *handle = new ColumnFamilyHandle(this, DCHECK_NOTNULL(cfd));
            result->push_back(handle);
        }
    }
    return rs;
    // Unlocking versions-------------------------------------------------------
}
    
/*virtual*/ Error
DBImpl::DropColumnFamilies(const std::vector<ColumnFamily *> &column_families) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
/*virtual*/ Error DBImpl::ReleaseColumnFamily(ColumnFamily *cf) {
    if (cf == nullptr) {
        return Error::OK();
    }
    ColumnFamilyHandle *handle = ColumnFamilyHandle::Cast(cf);
    if (this != handle->db()) {
        return MAI_CORRUPTION("Use difference db column family.");
    }
    delete handle;
    return Error::OK();
}
    
/*virtual*/ Error
DBImpl::GetAllColumnFamilies(std::vector<ColumnFamily *> *result) {
    DCHECK_NOTNULL(result)->clear();
    
    // Locking versions---------------------------------------------------------
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto impl : *versions_->column_families()) {
        if (!impl->IsDropped()) { // No dropped!
            ColumnFamily *handle = new ColumnFamilyHandle(this, impl);
            result->push_back(handle);
        }
    }
    return Error::OK();
    // Unlocking versions-------------------------------------------------------
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
    
    WritingHandler handler(last_version + 1, versions_->column_families(),
                           &mutex_);
    updates->Iterate(&handler); // Internal locking
    
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
    
Error DBImpl::Redo(uint64_t log_file_number,
                   core::SequenceNumber last_sequence_number) {
    std::unique_ptr<SequentialFile> file;
    std::string log_file_name = Files::LogFileName(db_name_, log_file_number);
    Error rs = env_->NewSequentialFile(log_file_name, &file);
    if (!rs) {
        return rs;
    }
    LogReader logger(file.get(), true, WAL::kDefaultBlockSize);
    
    std::string_view result;
    std::string scatch;
    WritingHandler handler(last_sequence_number + 1,
                           versions_->column_families(), &mutex_);
    while (logger.Read(&result, &scatch)) {
        rs = WriteBatch::Iterate(result.data(), result.size(), &handler);
        if (!rs) {
            return rs;
        }
    }
    if (!logger.error().ok() && !logger.error().IsEof()) {
        return logger.error();
    }
    versions_->AddSequenceNumber(handler.sequence_number_count());
    return Error::OK();
}
    
Error DBImpl::PrepareForGet(const ReadOptions &opts, ColumnFamily *cf,
                            GetContext *ctx) {
    if (cf == nullptr) {
        return MAI_CORRUPTION("NULL column family.");
    }
    ColumnFamilyHandle *handle = ColumnFamilyHandle::Cast(cf);
    if (this != handle->db()) {
        return MAI_CORRUPTION("Use difference db column family.");
    }
    ctx->cfd = handle->impl();
    if (ctx->cfd->IsDropped()) {
        return MAI_CORRUPTION("Column family has been dropped.");
    }
    
    ctx->last_sequence_number = 0;
    mutex_.lock(); // Locking versions------------------------------------------
    if (opts.snapshot) {
        ctx->last_sequence_number =
            SnapshotImpl::Cast(opts.snapshot)->sequence_number();
    } else {
        ctx->last_sequence_number = versions_->last_sequence_number();
    }
    
    ctx->in_mem.clear();
    ctx->in_mem.push_back(base::MakeRef(ctx->cfd->mutable_table()));
    // TODO:
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
    
Error DBImpl::InternalNewColumnFamily(const std::string &name,
                                      const ColumnFamilyOptions &options,
                                      uint32_t *cfid) {
    if (versions_->column_families()->GetColumnFamily(name) != nullptr) {
        return MAI_CORRUPTION("Duplicated column family name: " + name);
    }
    
    uint32_t new_id = versions_->column_families()->NextColumnFamilyId();
    VersionPatch patch;
    patch.AddColumnFamily(name, new_id, options.comparator->Name());
    Error rs = versions_->LogAndApply(options, &patch, &mutex_);
    if (!rs) {
        return rs;
    }
    *cfid = new_id;
    return Error::OK();
}
    
Error DBImpl::MakeRoomForWrite(ColumnFamilyImpl *cf, bool force) {
    // TODO:
    return Error::OK();
}
    
} // namespace db
    
const char kDefaultColumnFamilyName[] = "default";
    
/*static*/ Error DB::Open(const Options &opts,
                          const std::string name,
                          const std::vector<ColumnFamilyDescriptor> &descriptors,
                          std::vector<ColumnFamily *> *column_families,
                          DB **result) {
    if (name.empty()) {
        return MAI_CORRUPTION("Empty db name.");
    }
    db::DBImpl *impl = new db::DBImpl(name, opts.env);
    Error rs = impl->Open(opts, descriptors, column_families);
    if (!rs) {
        return rs;
    }
    
    *result = impl;
    return Error::OK();
}
    
} // namespace mai
