#include "db/db-impl.h"
#include "db/write-ahead-log.h"
#include "db/version.h"
#include "db/column-family.h"
#include "db/table-cache.h"
#include "db/files.h"
#include "db/factory.h"
#include "db/table-cache.h"
#include "db/config.h"
#include "db/compaction.h"
#include "db/snapshot-impl.h"
#include "db/db-iterator.h"
#include "table/table-builder.h"
#include "core/key-boundle.h"
#include "core/memory-table.h"
#include "core/merging.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "glog/logging.h"
#include <thread>

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

    

DBImpl::DBImpl(const std::string &db_name, const Options &opts)
    : db_name_(db_name)
    , env_(DCHECK_NOTNULL(opts.env))
    , abs_db_path_(env_->GetAbsolutePath(db_name))
    , factory_(Factory::NewDefault())
    , background_active_(0)
    , shutting_down_(false)
    , table_cache_(new TableCache(abs_db_path_, opts, factory_.get()))
    , versions_(new VersionSet(abs_db_path_, opts, table_cache_.get())) {
}

DBImpl::~DBImpl() {
    default_cf_.reset(); // Release default column family handle first.
    
    DLOG(INFO) << "Shutting down, last_version: "
               << versions_->last_sequence_number();
    {
        shutting_down_.store(true);
        
        std::unique_lock<std::mutex> lock(mutex_);
        while (background_active_.load() > 0) {
            for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
                if (cfd->background_progress()) {
                    cfd->mutable_background_cv()->wait(lock);
                }
            }
        }
    }
    
    // TODO: clean others
}
    
Error DBImpl::Open(const Options &opts,
                   const std::vector<ColumnFamilyDescriptor> &desc,
                   std::vector<ColumnFamily *> *result) {
    Error rs;

    rs = env_->FileExists(Files::CurrentFileName(abs_db_path_));
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
    if (rs.ok() && result) {
        ColumnFamilySet *column_familes = versions_->column_families();
        
        result->clear();
        for (const auto &d : desc) {
            ColumnFamilyImpl *cfd = column_familes->GetColumnFamily(d.name);
            DCHECK_NOTNULL(cfd);
            result->push_back(new ColumnFamilyHandle(this, cfd));
        }
        ColumnFamily *cf =
            new ColumnFamilyHandle(this, column_familes->GetDefault());
        default_cf_.reset(cf);
    }
    return rs;
}
    
Error DBImpl::NewDB(const Options &opts,
                    const std::vector<ColumnFamilyDescriptor> &desc) {
    Error rs = env_->MakeDirectory(abs_db_path_, true);
    if (!rs) {
        return rs;
    }
    rs = RenewLogger();
    if (!rs) {
        return rs;
    }

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
        versions_->column_families()->NewColumnFamily(opt.second, opt.first,
                                                      cfid, versions_.get());
    }
    for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
        rs = cfd->Install(factory_.get());
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
    std::string current_file_name = Files::CurrentFileName(abs_db_path_);
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

    if (opts.create_missing_column_families) {
        for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
            cf_opts.erase(cfd->name());
        }
        for (const auto &remain : cf_opts) {
            uint32_t cfid;
            rs = InternalNewColumnFamily(remain.first, remain.second, &cfid);
            if (!rs) {
                return rs;
            }
        }
    }
    for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
        if (!cfd->initialized()) {
            rs = cfd->Install(factory_.get());
            if (!rs) {
                return rs;
            }
        }
        DLOG(INFO) << "Column family: " << cfd->name() << " installed.";
    }
    
    rs = Redo(versions_->redo_log_number(), logs[logs.size() - 2]);
    if (!rs) {
        return rs;
    }
    DLOG(INFO) << "Replay ok, last version: "
               << versions_->last_sequence_number();
    
    log_file_number_ = versions_->redo_log_number();
    rs = env_->NewWritableFile(Files::LogFileName(abs_db_path_,
                                                  log_file_number_), true,
                               &log_file_);
    if (!rs) {
        return rs;
    }
    logger_.reset(new LogWriter(log_file_.get(), WAL::kDefaultBlockSize));
    
    return Error::OK();
}
    
/*virtual*/ Error DBImpl::NewColumnFamily(const std::string &name,
                                          const ColumnFamilyOptions &options,
                                          ColumnFamily **result) {
    // Locking versions---------------------------------------------------------
    std::unique_lock<std::mutex> lock(mutex_);
    uint32_t cfid;
    Error rs = InternalNewColumnFamily(name, options, &cfid);
    if (!rs) {
        return rs;
    }
    ColumnFamilySet *cfs = versions_->column_families();
    *result = new ColumnFamilyHandle(this,
                                     DCHECK_NOTNULL(cfs->GetColumnFamily(cfid)));
    return rs;
    // Unlocking versions-------------------------------------------------------
}
    
/*virtual*/ Error DBImpl::DropColumnFamily(ColumnFamily *cf) {
    if (!cf) {
        return MAI_CORRUPTION("NULL column family.");
    }
    if (cf->id() == 0) { // TODO: focre delete default column family.
        return MAI_CORRUPTION("default column family can no be delete.");
    }
    ColumnFamilyHandle *handle = ColumnFamilyHandle::Cast(cf);
    if (this != handle->db()) {
        return MAI_CORRUPTION("Use difference db column family.");
    }
    ColumnFamilyImpl *cfd = handle->impl();
    if (cfd->dropped()) {
        return MAI_CORRUPTION("Column family has been dropped.");
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    while (cfd->background_progress()) {
        // Waiting for all background progress done
        cfd->mutable_background_cv()->wait(lock);
    }
    VersionPatch patch;
    patch.DropColumnFamily(cfd->id());
    Error rs = versions_->LogAndApply(cfd->options(), &patch, &mutex_);
    if (!rs) {
        return rs;
    }
    rs = cfd->Uninstall();
    if (!rs) {
        return rs;
    }
    
    delete handle;
    return Error::OK();
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
        if (!impl->dropped()) { // No dropped!
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
        
        for (auto cfd : *versions_->column_families()) {
            rs = MakeRoomForWrite(cfd, false);
            if (!rs) {
                return rs;
            }
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
    
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_ptr<Iterator> internal(NewInternalIterator(opts, ctx.cfd));
    if (internal->error().fail()) {
        return internal.release();
    }
    
    return new DBIterator(ctx.cfd->ikcmp()->ucmp(), internal.release(),
                          ctx.last_sequence_number);
}
    
/*virtual*/ const Snapshot *DBImpl::GetSnapshot() {
    std::unique_lock<std::mutex> lock(mutex_);
    return snapshots_.NewSnapshot(versions_->last_sequence_number());
}
    
/*virtual*/ void DBImpl::ReleaseSnapshot(const Snapshot *snapshot) {
    std::unique_lock<std::mutex> lock(mutex_);
    const SnapshotImpl *impl = SnapshotImpl::Cast(snapshot);
    snapshots_.DeleteSnapshot(const_cast<SnapshotImpl *>(impl));
}
    
/*virtual*/ ColumnFamily *DBImpl::DefaultColumnFamily() {
    return default_cf_.get();
}
    
Iterator *DBImpl::NewInternalIterator(const ReadOptions &opts,
                                      ColumnFamilyImpl *cfd) {
    std::vector<Iterator *> iters;
    iters.push_back(cfd->mutable_table()->NewIterator());
    
    std::vector<base::Handle<core::MemoryTable>> in_mem;
    cfd->immutable_pipeline()->PeekAll(&in_mem);
    for (auto memtable : in_mem) {
        iters.push_back(memtable->NewIterator());
    }

    Error rs = cfd->AddIterators(opts, &iters);
    if (!rs) {
        for (auto clean : iters) {
            delete clean;
        }
        return Iterator::AsError(rs);
    }

    Iterator *internal = core::Merging::NewMergingIterator(cfd->ikcmp(),
                                                           &iters[0],
                                                           iters.size());
    // No need register cleanup:
    // Memory table's iterator can cleanup itself reference count.
    return internal;
}
    
void DBImpl::TEST_MakeImmutablePipeline(ColumnFamily *cf) {
    ColumnFamilyImpl *cfd = DCHECK_NOTNULL(ColumnFamilyHandle::Cast(cf)->impl());
    cfd->MakeImmutablePipeline(factory_.get());
}

Error DBImpl::TEST_ForceDumpImmutableTable(ColumnFamily *cf, bool sync) {
    ColumnFamilyImpl *cfd = DCHECK_NOTNULL(ColumnFamilyHandle::Cast(cf)->impl());
    DCHECK_EQ(0, versions_->prev_log_number());
    Error rs = RenewLogger();
    if (!rs) {
        return rs;
    }
    MaybeScheduleCompaction(cfd);
    
    std::unique_lock<std::mutex> lock(mutex_);
    if (sync && cfd->background_progress()) {
        cfd->mutable_background_cv()->wait(lock);
    }
    return Error::OK();
}
    
Error DBImpl::RenewLogger() {
    uint64_t new_log_number = versions_->GenerateFileNumber();
    std::string log_file_name = Files::LogFileName(abs_db_path_,
                                                   new_log_number);
    Error rs = env_->NewWritableFile(log_file_name, true, &log_file_);
    if (!rs) {
        return rs;
    }
    log_file_number_ = new_log_number;
    logger_.reset(new LogWriter(log_file_.get(), WAL::kDefaultBlockSize));
    return Error::OK();
}
    
Error DBImpl::Redo(uint64_t log_file_number,
                   core::SequenceNumber last_sequence_number) {
    std::unique_ptr<SequentialFile> file;
    std::string log_file_name = Files::LogFileName(abs_db_path_, log_file_number);
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
    if (ctx->cfd->dropped()) {
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
    ctx->cfd->immutable_pipeline()->PeekAll(&ctx->in_mem);
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
    if (cfd->dropped()) {
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
    
// REQUIRES mutex_.lock()
Error DBImpl::MakeRoomForWrite(ColumnFamilyImpl *cfd, bool force) {
    Error rs;
    
    while (true) {
        
        if (cfd->background_error().fail()) {
            rs = cfd->background_error();
            //cfd->set_background_error(Error::OK());
            break;
        } else if (cfd->mutable_table()->ApproximateMemoryUsage() <
                   cfd->options().write_buffer_size) {
            // Memory table usage samll than write buffer, Ignore it.
            break;
        } else if (cfd->immutable_pipeline()->InProgress()) {
            // Immutable table pipeline in progress.
            break;
        } else if (cfd->background_progress() &&
                   cfd->current()->level_files(0).size() >
                   Config::kMaxNumberLevel0File) {
            // TODO:
            break;
        } else {
            DCHECK_EQ(0, versions_->prev_log_number());
            rs = RenewLogger();
            if (!rs) {
                break;
            }
            cfd->MakeImmutablePipeline(factory_.get());
            MaybeScheduleCompaction(cfd);
        }
        
        // TODO:
    }
    return rs;
}

// REQUIRES mutex_.lock()
void DBImpl::MaybeScheduleCompaction(ColumnFamilyImpl *cfd) {
    if (background_active_.load() > 0) {
        return; // Compaction is running.
    }
    
    if (shutting_down_.load()) {
        return; // Is shutting down, ignore schedule
    }
    
    if (!cfd->immutable_pipeline()->InProgress() &&
        !cfd->NeedsCompaction()) {
        return; // Compaction is no need
    }
    
    background_active_.fetch_add(1);
    cfd->set_background_progress(true);
    std::thread([this](auto cfd) {
        this->BackgroundWork(cfd);
    }, cfd).detach();
}
    
void DBImpl::BackgroundWork(ColumnFamilyImpl *cfd) {
    DLOG(INFO) << "Background work on. " << cfd->name();
    
    DCHECK_GT(background_active_.load(), 0);
    if (!shutting_down_.load()) {
        std::unique_lock<std::mutex> lock(mutex_);
        BackgroundCompaction(cfd);
    }
    background_active_.fetch_sub(1);
    cfd->set_background_progress(false);
    
    MaybeScheduleCompaction(cfd);
    cfd->mutable_background_cv()->notify_all();
}

// REQUIRES mutex_.lock()
void DBImpl::BackgroundCompaction(ColumnFamilyImpl *cfd) {
    
    if (cfd->immutable_pipeline()->InProgress()) {
        Error rs = CompactMemoryTable(cfd);
        if (rs.fail()) {
            DLOG(INFO) << "Compact memory table fail! column family: "
                       << cfd->name() << " cause: " << rs.ToString();
            cfd->set_background_error(rs);
            return;
        }
    }

    CompactionContext ctx;
    if (cfd->PickCompaction(&ctx)) {
        Error rs = CompactFileTable(cfd, &ctx);
        if (rs.fail()) {
            DLOG(INFO) << "Compact file table fail! column family: "
                       << cfd->name() << " cause: " << rs.ToString();
            cfd->set_background_error(rs);
            return;
        }
        rs = versions_->LogAndApply(ColumnFamilyOptions{}, &ctx.patch, &mutex_);
        if (!rs) {
            cfd->set_background_error(rs);
            return;
        }
    }
    
    DeleteObsoleteFiles(cfd);
}

// REQUIRES mutex_.lock()
Error DBImpl::CompactMemoryTable(ColumnFamilyImpl *cfd) {
    DCHECK(cfd->immutable_pipeline()->InProgress());
    
    Error rs;
    VersionPatch patch;
    base::Handle<core::MemoryTable> imm;
    while (cfd->immutable_pipeline()->Take(&imm)) {
        DCHECK(!imm.is_null());
        rs = WriteLevel0Table(cfd->current(), &patch, imm.get());
        if (!rs) {
            return rs;
        }
        if (shutting_down_.load()) {
            return MAI_IO_ERROR("Deleting DB during memtable compaction");
        }
    }

    patch.set_prev_log_number(0);
    patch.set_redo_log_number(log_file_number_);
    rs = versions_->LogAndApply(ColumnFamilyOptions{}, &patch, &mutex_);
    if (!rs) {
        return rs;
    }
    return Error::OK();
}

// REQUIRES mutex_.lock()
Error DBImpl::CompactFileTable(ColumnFamilyImpl *cfd, CompactionContext *ctx) {
    DCHECK_GE(ctx->level, 0);
    DCHECK_LT(ctx->level, Config::kMaxLevel - 1);
    
    std::unique_ptr<Compaction>
    job(factory_->NewCompaction(abs_db_path_, cfd->ikcmp(),
                                table_cache_.get(), cfd));
    job->set_target_level(ctx->level + 1);
    job->set_compaction_point(cfd->compaction_point(ctx->level));
    job->set_oldest_sequence_number(versions_->last_sequence_number());
    job->set_target_file_number(versions_->GenerateFileNumber());
    
    for (auto fmd : ctx->inputs[0]) {
        Iterator *iter = table_cache_->NewIterator(ReadOptions{}, cfd,
                                                   fmd->number, fmd->size);
        Error rs = iter->error();
        if (!rs) {
            delete iter;
            return rs;
        }
        job->AddInput(iter);
        ctx->patch.DeleteFile(cfd->id(), ctx->level, fmd->number);
    }
    for (auto fmd : ctx->inputs[1]) {
        Iterator *iter = table_cache_->NewIterator(ReadOptions{}, cfd,
                                                   fmd->number, fmd->size);
        Error rs = iter->error();
        if (!rs) {
            delete iter;
            return rs;
        }
        job->AddInput(iter);
        ctx->patch.DeleteFile(cfd->id(), ctx->level + 1, fmd->number);
    }
    
    std::unique_ptr<WritableFile> file;
    Error rs =
        env_->NewWritableFile(cfd->GetTableFileName(job->target_file_number()),
                              false, &file);
    if (!rs) {
        return rs;
    }
    std::unique_ptr<table::TableBuilder>
    builder(factory_->NewTableBuilder(cfd->ikcmp(),
                                      cfd->options().use_unordered_table,
                                      file.get(),
                                      cfd->options().block_size,
                                      cfd->options().block_restart_interval,
                                      cfd->options().number_of_hash_slots));
    CompactionResult result;
    mutex_.unlock();
    rs = job->Run(builder.get(), &result);
    mutex_.lock();
    if (!rs) {
        builder->Abandon();
        return rs;
    }
    mutex_.unlock();
    rs = builder->Finish();
    mutex_.lock();
    if (!rs) {
        builder->Abandon();
        return rs;
    }
    
    FileMetaData *fmd = new FileMetaData(job->target_file_number());
    fmd->ctime        = env_->CurrentTimeMicros();
    fmd->size         = builder->FileSize();
    fmd->largest_key  = result.largest_key;
    fmd->smallest_key = result.smallest_key;
    ctx->patch.CreaetFile(cfd->id(), job->target_level(), fmd);
    return Error::OK();
}
    
Error DBImpl::WriteLevel0Table(Version *current, VersionPatch *patch,
                               core::MemoryTable *table) {
    base::Handle<FileMetaData>
        fmd(new FileMetaData(versions_->GenerateFileNumber()));
    std::unique_ptr<Iterator> iter(table->NewIterator());
    if (iter->error().fail()) {
        return iter->error();
    }
    LOG(INFO) << "Level0 table compaction start, target file number: "
              << fmd->number;
    
    mutex_.unlock(); // Do not need DB lock -------------------------------------
    
    std::unique_ptr<WritableFile> file;
    std::string table_file_name = current->owns()->GetTableFileName(fmd->number);
    Error rs = env_->NewWritableFile(table_file_name, false, &file);
    if (!rs) {
        mutex_.lock();
        return rs;
    }
    ColumnFamilyImpl *cfd = current->owns();
    std::unique_ptr<table::TableBuilder>
        builder(factory_->NewTableBuilder(cfd->ikcmp(),
                                          cfd->options().use_unordered_table,
                                          file.get(), cfd->options().block_size,
                                          cfd->options().block_restart_interval,
                                          cfd->options().number_of_hash_slots));
    std::string largest_key, smallest_key;
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        builder->Add(iter->key(), iter->value());
        rs = builder->error();
        if (!rs) {
            builder->Abandon();
            mutex_.lock();
            return rs;
        }
        if (largest_key.empty() ||
            cfd->ikcmp()->Compare(iter->key(), largest_key) > 0) {
            largest_key = iter->key();
        }
        if (smallest_key.empty() ||
            cfd->ikcmp()->Compare(iter->key(), smallest_key) < 0) {
            smallest_key = iter->key();
        }
    }
    rs = builder->Finish();
    if (!rs) {
        mutex_.lock();
        return rs;
    }
    
    fmd->ctime        = env_->CurrentTimeMicros();
    fmd->size         = builder->FileSize();
    fmd->largest_key  = largest_key;
    fmd->smallest_key = smallest_key;
    patch->CreaetFile(cfd->id(), 0, fmd.get());
    mutex_.lock();
    return Error::OK();
}
    
void DBImpl::DeleteObsoleteFiles(ColumnFamilyImpl *cfd) {
    std::vector<std::string> children;
    
    Error rs = env_->GetChildren(abs_db_path_, &children);
    if (!rs) {
        DLOG(ERROR) << "Can not get children, cause: " << rs.ToString();
        return;
    }
    
    std::map<uint64_t, std::string> cleanup;
    for (const auto &name : children) {
        uint64_t number;
        Files::Kind kind;
        std::tie(kind, number) = Files::ParseName(name);
        switch (kind) {
            case Files::kLog:
            case Files::kManifest:
                cleanup[number] = abs_db_path_ + "/" + name;
                break;
                
            default:
                break;
        }
    }
    cleanup.erase(log_file_number_);
    cleanup.erase(versions_->redo_log_number());
    cleanup.erase(versions_->manifest_file_number());
    
    rs = env_->GetChildren(cfd->GetDir(), &children);
    for (const auto &name : children) {
        uint64_t number;
        Files::Kind kind;
        std::tie(kind, number) = Files::ParseName(name);
        switch (kind) {
            case Files::kSST_Table:
            case Files::kS1T_Table:
            case Files::kXMT_Table:
                cleanup[number] = cfd->GetDir() + "/" + name;
                break;

            default:
                break;
        }
    }
    for (int i = 0; i < Config::kMaxLevel; ++i) {
        for (auto fmd : cfd->current()->level_files(i)) {
            cleanup.erase(fmd->number);
        }
    }
    
    for (const auto &pair : cleanup) {
        rs = env_->DeleteFile(pair.second, false);
        if (!rs) {
            DLOG(ERROR) << "Delete obsolete file: " << pair.second << " fail!"
                        << " cause: " << rs.ToString();
        } else {
            DLOG(INFO) << "Delete obsolete file: " << pair.second;
        }
        table_cache_->Invalidate(pair.first);
    }
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
    db::DBImpl *impl = new db::DBImpl(name, opts);
    Error rs = impl->Open(opts, descriptors, column_families);
    if (!rs) {
        return rs;
    }
    
    *result = impl;
    return Error::OK();
}
    
/*virtual*/ Error
DB::NewColumnFamilies(const std::vector<std::string> &names,
                      const ColumnFamilyOptions &options,
                      std::vector<ColumnFamily *> *result) {
    DCHECK_NOTNULL(result)->clear();
    Error rs;
    for (const std::string &name : names) {
        ColumnFamily *cf;
        rs = NewColumnFamily(name, options, &cf);
        if (!rs) {
            for (auto handle : *result) {
                ReleaseColumnFamily(handle);
            }
            break;
        }
        result->push_back(cf);
    }
    return rs;
}
    
/*virtual*/ Error
DB::DropColumnFamilies(const std::vector<ColumnFamily *> &column_families) {
    Error rs;
    for (auto cf : column_families) {
        rs = ReleaseColumnFamily(cf);
        if (!rs) {
            break;
        }
    }
    return rs;
}
    
} // namespace mai
