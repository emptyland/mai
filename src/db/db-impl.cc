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
#include "table/table.h"
#include "table/block-cache.h"
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
    WritingHandler(uint64_t redo_log_number, bool filter,
                   ColumnFamilySet *column_families)
        : redo_log_number_(redo_log_number)
        , filter_(filter)
        , column_families_(DCHECK_NOTNULL(column_families)) {}
    
    virtual ~WritingHandler() {}
    
    virtual void Put(uint32_t cfid, std::string_view key,
                     std::string_view value) override {
        base::intrusive_ptr<core::MemoryTable> table;
        EnsureGetTable(cfid, &table);
        
        if (!table.is_null()) {
            table->Put(key, value, sequence_number(), core::Tag::kFlagValue);
            
            size_count_ += key.size() + sizeof(uint32_t) + sizeof(uint64_t);
            size_count_ += value.size();
        }
        sequence_number_count_ ++;
    }
    
    virtual void Delete(uint32_t cfid, std::string_view key) override {
        base::intrusive_ptr<core::MemoryTable> table;
        EnsureGetTable(cfid, &table);

        if (!table.is_null()) {
            table->Put(key, "", sequence_number(), core::Tag::kFlagDeletion);
            size_count_ += key.size() + sizeof(uint32_t) + sizeof(uint64_t);
        }
        sequence_number_count_ ++;
    }
    
    core::SequenceNumber sequence_number() const {
        return last_sequence_number_ + sequence_number_count_;
    }
    
    bool EnsureGetTable(uint32_t cfid, base::intrusive_ptr<core::MemoryTable> *table) {
        ColumnFamilyImpl *impl = (cfid == 0) ? column_families_->GetDefault() :
            EnsureGetColumnFamily(cfid);
        if (filter_ && impl->redo_log_number() < redo_log_number_) {
            *table = nullptr;
        } else {
            *table = impl->mutable_table();
        }
        return true;
    }
    
    ColumnFamilyImpl *EnsureGetColumnFamily(uint32_t cfid) {
        ColumnFamilyImpl *impl = (cfid == 0) ? column_families_->GetDefault() :
            column_families_->GetColumnFamily(cfid);
        DCHECK(impl != nullptr);
        DCHECK(impl->initialized());
        return impl;
    }
    
    DEF_VAL_GETTER(uint64_t, size_count);
    DEF_VAL_GETTER(uint64_t, sequence_number_count);
    
    void ResetLastSequenceNumber(core::SequenceNumber sn) {
        last_sequence_number_  = sn;
        sequence_number_count_ = 0;
    }

private:
    const uint64_t redo_log_number_;
    const bool filter_;
    ColumnFamilySet *const column_families_;
    
    core::SequenceNumber last_sequence_number_;
    uint64_t size_count_ = 0;
    uint64_t sequence_number_count_ = 0;
}; // class WritingHnalder
    

struct GetContext {
    std::vector<base::intrusive_ptr<core::MemoryTable>> in_mem;
    core::SequenceNumber                  last_sequence_number;
    base::intrusive_ptr<ColumnFamilyImpl> cfd;
    Version                              *current;
}; // struct ReadContext


static inline void MakeRedo(std::string *buf, core::SequenceNumber sn,
                            uint32_t n_entries) {
    buf->append(reinterpret_cast<const char *>(&sn), sizeof(sn));
    buf->append(reinterpret_cast<const char *>(&n_entries), sizeof(n_entries));
}

DBImpl::DBImpl(const std::string &db_name, const Options &opts)
    : db_name_(db_name)
    , options_(opts)
    , env_(DCHECK_NOTNULL(opts.env))
    , abs_db_path_(env_->GetAbsolutePath(db_name))
    , factory_(Factory::NewDefault())
    , bkg_active_(0)
    , shutting_down_(false)
    , table_cache_(new TableCache(abs_db_path_, opts, factory_.get()))
    , versions_(new VersionSet(abs_db_path_, opts, table_cache_.get()))
    , flush_request_(0)
    , total_wal_size_(0) {
}

DBImpl::~DBImpl() {
    default_cf_.reset(); // Release default column family handle first.
    
    DLOG(INFO) << "Shutting down, last_version: "
               << versions_->last_sequence_number();
    {
        shutting_down_.store(true);
        
        std::unique_lock<std::mutex> lock(mutex_);
        while (bkg_active_.load() > 0) {
            for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
                if (cfd->background_progress()) {
                    cfd->mutable_background_cv()->wait(lock);
                }
            }
        }
    }
    
    bkg_cv_.notify_all();
    flush_worker_.join();

    // TODO: clean others
}
    
Error DBImpl::Open(const std::vector<ColumnFamilyDescriptor> &desc,
                   std::vector<ColumnFamily *> *result) {
    Error rs;

    rs = env_->FileExists(Files::CurrentFileName(abs_db_path_));
    if (rs.fail()) { // Not exists
        if (!options_.create_if_missing) {
            return MAI_CORRUPTION("db miss and create_if_missing is false.");
        }
        rs = NewDB(desc);
    } else {
        if (options_.error_if_exists) {
            return MAI_CORRUPTION("db exists and error_if_exists is true.");
        }
        rs = Recovery(desc);
    }
    ColumnFamilySet *column_familes = versions_->column_families();
    if (rs.ok() && result) {
        result->clear();
        for (const auto &d : desc) {
            ColumnFamilyImpl *cfd = column_familes->GetColumnFamily(d.name);
            result->push_back(new ColumnFamilyHandle(this, DCHECK_NOTNULL(cfd)));
        }
    }
    if (rs.ok()) {
        ColumnFamily *cf = new ColumnFamilyHandle(this, column_familes->GetDefault());
        default_cf_.reset(cf);
    }
    
    flush_worker_ = std::thread([&](){ this->FlushWork(); });
    return rs;
}

Error DBImpl::NewDB(const std::vector<ColumnFamilyDescriptor> &desc) {
    Error rs = env_->FileExists(abs_db_path_);
    if (!rs) {
        rs = env_->MakeDirectory(abs_db_path_, true);
    }
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
        versions_->column_families()->NewColumnFamily(options_, kDefaultColumnFamilyName, 0,
                                                      versions_.get());
    } else {
        versions_->column_families()->NewColumnFamily(iter->second, kDefaultColumnFamilyName, 0,
                                                      versions_.get());
    }
    for (const auto &opt : cf_opts) {
        if (opt.first.compare(kDefaultColumnFamilyName) == 0) {
            continue;
        }
        uint32_t cfid = versions_->column_families()->NextColumnFamilyId();
        versions_->column_families()->NewColumnFamily(opt.second, opt.first, cfid, versions_.get());
    }
    for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
        rs = cfd->Install(factory_.get());
        if (!rs) {
            return rs;
        }
        cfd->set_redo_log_number(log_file_number_);
    }
    VersionPatch patch;
    patch.SetPrevLogNumber(0);
    patch.SetRedoLogNumber(log_file_number_);
    versions_->AddSequenceNumber(1); // Begin with 1
    return versions_->LogAndApply(options_, &patch, &mutex_);
}

Error DBImpl::Recovery(const std::vector<ColumnFamilyDescriptor> &desc) {
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
    std::map<std::string, ColumnFamilyOptions> cf_opts;
    for (const auto &d : desc) {
        cf_opts[d.name] = d.options;
    }
    std::set<uint64_t> history;
    rs = versions_->Recovery(cf_opts, manifest_file_number, &history);
    if (!rs) {
        return rs;
    }
    DCHECK_GE(history.size(), 1);

    if (options_.create_missing_column_families) {
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
    
    // Should replay all column families's redo log file.
    std::set<uint64_t> numbers;
    uint64_t max_number = 0;
    for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
        if (!cfd->initialized()) {
            rs = cfd->Install(factory_.get());
            if (!rs) {
                return rs;
            }
        }
        DLOG(INFO) << "Column family: " << cfd->name() << " installed.";
        numbers.insert(cfd->redo_log_number());
        if (cfd->redo_log_number() > max_number) {
            max_number = cfd->redo_log_number();
        }
    }
    // Add undumped log files.
    for (auto iter = history.upper_bound(max_number); iter != history.end();
         iter++) {
        numbers.insert(*iter);
        max_number = *iter;
    }
    
    DCHECK_GE(history.size(), numbers.size());
    core::SequenceNumber last = 0, update = 0;
    for (uint64_t number: numbers) {
        //last = history[number];
        // The newest redo log file filter is not need.
        rs = Redo(number, last, &update, max_number != number);
        if (!rs) {
            return rs;
        }
    }
    // The max one is newest file.
    DCHECK_EQ(max_number, versions_->redo_log_number());
    
    log_file_number_ = versions_->redo_log_number();
    //DCHECK_GE(update, versions_->last_sequence_number());
    
    versions_->UpdateSequenceNumber(update);
    DLOG(INFO) << "Replay ok, last version: "
               << versions_->last_sequence_number();

    rs = env_->NewWritableFile(Files::LogFileName(abs_db_path_, log_file_number_), true, &log_file_);
    if (!rs) {
        return rs;
    }
    logger_.reset(new LogWriter(log_file_.get(), WAL::kDefaultBlockSize));
    
    uint64_t wal_size = 0;
    rs = GetTotalWalSize(&wal_size);
    if (!rs) {
        return rs;
    }
    total_wal_size_.store(wal_size, std::memory_order_release);
    return Error::OK();
}
    
/*virtual*/ Error DBImpl::NewColumnFamily(const std::string &name,
                                          const ColumnFamilyOptions &options,
                                          ColumnFamily **result) {
    // Locking versions-----------------------------------------------------------------------------
    std::unique_lock<std::mutex> lock(mutex_);
    uint32_t cfid;
    Error rs = InternalNewColumnFamily(name, options, &cfid);
    if (!rs) {
        return rs;
    }
    ColumnFamilySet *cfs = versions_->column_families();
    ColumnFamilyImpl *cfd = DCHECK_NOTNULL(cfs->GetColumnFamily(cfid));
    rs = cfd->Install(factory_.get());
    if (!rs) {
        return rs;
    }
    cfd->set_redo_log_number(log_file_number_);
    DCHECK(cfd->initialized());
    *result = new ColumnFamilyHandle(this, cfd);
    return rs;
    // Unlocking versions---------------------------------------------------------------------------
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
    
    // Locking versions-----------------------------------------------------------------------------
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto impl : *versions_->column_families()) {
        if (!impl->dropped()) { // No dropped!
            ColumnFamily *handle = new ColumnFamilyHandle(this, impl);
            result->push_back(handle);
        }
    }
    return Error::OK();
    // Unlocking versions---------------------------------------------------------------------------
}
    
/*virtual*/ Error DBImpl::Put(const WriteOptions &opts, ColumnFamily *cf,
                  std::string_view key, std::string_view value) {
    WriteBatch batch;
    batch.Put(cf, key, value);
    return Write(opts, &batch);
    //return Write(opts, cf, key, value, core::Tag::kFlagValue);
}
    
/*virtual*/ Error DBImpl::Delete(const WriteOptions &opts, ColumnFamily *cf,
                     std::string_view key) {
    WriteBatch batch;
    batch.Delete(cf, key);
    return Write(opts, &batch);
    //return Write(opts, cf, key, "", core::Tag::kFlagDeletion);
}
    
/*virtual*/ Error DBImpl::Write(const WriteOptions& opts, WriteBatch* updates) {
    return WriteImpl(opts, updates, nullptr);
}
    
/*virtual*/ Error DBImpl::Get(const ReadOptions &opts, ColumnFamily *cf, std::string_view key,
                              std::string *value) {
    
    using core::Tag;
    
    GetContext ctx;
    core::Tag tag;
    Error rs = PrepareForGet(opts, cf, &ctx);
    for (const auto &table : ctx.in_mem) {
        rs = table->Get(key, ctx.last_sequence_number, &tag, value);
        if (rs.ok()) {
            if (tag.flag() == core::Tag::kFlagDeletion) {
                rs = MAI_NOT_FOUND("Deleted.");
            }
            return rs;
        }
    }
    rs = ctx.current->Get(opts, key, ctx.last_sequence_number, &tag, value);
    if (!rs) {
        return rs;
    }
    if (tag.flag() == core::Tag::kFlagDeletion) {
        return MAI_NOT_FOUND("Deleted.");
    }
    return rs;
}
    
/*virtual*/ Iterator *
DBImpl::NewIterator(const ReadOptions &opts, ColumnFamily *cf) {
    GetContext ctx;
    Error rs = PrepareForGet(opts, cf, &ctx);
    if (!rs) {
        return Iterator::AsError(rs);
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_ptr<Iterator> internal(NewInternalIterator(opts, ctx.cfd.get()));
    if (internal->error().fail()) {
        return internal.release();
    }
    
    return new DBIterator(ctx.cfd->ikcmp()->ucmp(), internal.release(),
                          ctx.last_sequence_number);
}
    
/*virtual*/ const Snapshot *DBImpl::GetSnapshot() {
    std::unique_lock<std::mutex> lock(mutex_);
    return snapshots_.NewSnapshot(versions_->last_sequence_number(),
                                  env_->CurrentTimeMicros() / 1000);
}
    
/*virtual*/ void DBImpl::ReleaseSnapshot(const Snapshot *snapshot) {
    std::unique_lock<std::mutex> lock(mutex_);
    const SnapshotImpl *impl = SnapshotImpl::Cast(snapshot);
    snapshots_.DeleteSnapshot(const_cast<SnapshotImpl *>(impl));
}
    
/*virtual*/ ColumnFamily *DBImpl::DefaultColumnFamily() { return default_cf_.get(); }
    
/*virtual*/ Error DBImpl::GetProperty(std::string_view property,
                                      std::string *value) {
    using ::mai::base::Slice;
    using ::mai::core::KeyBoundle;
    
    DCHECK_NOTNULL(value)->clear();
    
    if (property == "db.log.current-name") {
        
        std::unique_lock<std::mutex> lock(mutex_);
        *value = Files::LogFileName(abs_db_path_, log_file_number_);
    } else if (property == "db.log.active") {
        
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto cfd : *versions_->column_families()) {
            if (cfd->dropped()) {
                continue;
            }
            value->append(base::Sprintf("%llu,", cfd->redo_log_number()));
        }
        value->erase(value->size() - 1, 1);
    } else if (property == "db.log.total-size") {
        
        *value = base::Sprintf("%" PRIu64, total_wal_size_.load(std::memory_order_acquire));
    } else if (property == "db.bkg.jobs") {
        
        *value = base::Sprintf("%d", bkg_active_.load());
    } else if (property == "db.versions.last-sequence-number") {
        
        std::unique_lock<std::mutex> lock(mutex_);
        *value = base::Sprintf("%" PRIu64, versions_->last_sequence_number());
    } else if (property.find("db.cf.") == 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        property.remove_prefix(6);
        size_t n = property.find('.');
        std::string cf_name(property.substr(0, n));
        ColumnFamilyImpl *cfd = versions_->column_families()->GetColumnFamily(cf_name);
        if (!cfd) {
            return MAI_CORRUPTION(base::Sprintf("Column family: %s not found.",
                                                cf_name.c_str()));
        }
        property.remove_prefix(cf_name.size() + 1);
        
        if (property == "levels") {
            for (int i = 0; i < Config::kMaxLevel; ++i) {
                value->append("+--------------------------------------------+\n");
                value->append(base::Sprintf("|== Level: %d ==\n", i));
                for (auto fmd : cfd->current()->level_files(i)) {
                    value->append(base::Sprintf("+- Number: [%" PRIu64 "]\n", fmd->number));
                    value->append("|---- Smallest : ");
                    value->append(KeyBoundle::ToString(fmd->smallest_key));
                    value->append("\n");
                    value->append("|---- Largest  : ");
                    value->append(KeyBoundle::ToString(fmd->largest_key));
                    value->append("\n");
                }
            }
            value->append("+--------------------------------------------+\n");
        }
    } else {
        return MAI_CORRUPTION(base::Sprintf("Incorrect property name: %.*s", int(property.size()),
                                            property.data()));
    }

    return Error::OK();
}
    
Error DBImpl::WriteImpl(const WriteOptions& opts, WriteBatch* batch,
                        WriteCallback *callback) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    core::SequenceNumber last_version = versions_->last_sequence_number();
    Error rs;
    for (auto cfd : *versions_->column_families()) {
        rs = MakeRoomForWrite(cfd, &lock);
        if (!rs) {
            return rs;
        }
    }
    if (callback) {
        rs = callback->Prepare(this);
        if (!rs) {
            return rs;
        }
    }
    
    rs = logger_->Append(batch->redo(last_version + 1));
    if (!rs) {
        return rs;
    }
    flush_request_.fetch_add(1);
    
    if (opts.sync) {
        rs = logger_->Flush();
        if (!rs) {
            return rs;
        }
        rs = logger_->Sync(true);
        if (!rs) {
            return rs;
        }
    }
    if (callback) {
        callback->WALDone(this);
    }
    
    WritingHandler handler(0, false, versions_->column_families());
    handler.ResetLastSequenceNumber(last_version + 1);
    batch->Iterate(&handler);
    
    versions_->AddSequenceNumber(handler.sequence_number_count());
    
    if (callback) {
        callback->Done(this);
    }
    return Error::OK();
}
    
Iterator *DBImpl::NewInternalIterator(const ReadOptions &opts,
                                      ColumnFamilyImpl *cfd) {
    std::vector<Iterator *> iters;
    iters.push_back(cfd->mutable_table()->NewIterator());
    
    std::vector<base::intrusive_ptr<core::MemoryTable>> in_mem;
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

    Iterator *internal = core::Merging::NewMergingIterator(cfd->ikcmp(), &iters[0], iters.size());
    // No need register cleanup:
    // Memory table's iterator can cleanup itself reference count.
    return internal;
}
    
core::SequenceNumber DBImpl::GetLatestSequenceNumber() {
    std::unique_lock<std::mutex> lock(mutex_);
    return versions_->last_sequence_number();
}
    
Error DBImpl::GetColumnFamilyImpl(uint32_t cfid, base::intrusive_ptr<ColumnFamilyImpl> *result) {
    
    auto impl = versions_->column_families()->GetColumnFamily(cfid);
    if (!impl) {
        return MAI_CORRUPTION("Column family not found!");
    }
    result->reset(impl);
    return Error::OK();
}
    
Error DBImpl::GetLatestSequenceForKey(ColumnFamilyImpl *impl, bool cache_only,
                              std::string_view key, core::SequenceNumber *seq) {
    auto curr_seq = versions_->last_sequence_number();
    *seq = core::Tag::kMaxSequenceNumber;
    
    std::string value;
    core::Tag tag;
    Error rs = impl->mutable_table()->Get(key, curr_seq, &tag, &value);
    if (rs.ok()) {
        *seq = tag.sequence_number();
        return Error::OK();
    }
    
    std::vector<base::intrusive_ptr<core::MemoryTable>> imm;
    impl->immutable_pipeline()->PeekAll(&imm);
    for (const auto &table : imm) {
        rs = table->Get(key, curr_seq, &tag, &value);
        if (rs.ok()) {
            *seq = tag.sequence_number();
            return Error::OK();
        }
    }
    if (cache_only) {
        return MAI_NOT_FOUND("No any key in memory tables!");
    }
    
    rs = impl->current()->Get(ReadOptions{}, key, curr_seq, &tag, &value);
    if (rs.ok()) {
        *seq = tag.sequence_number();
        return Error::OK();
    }
    return rs;
}

Error DBImpl::TEST_ForceDumpImmutableTable(ColumnFamily *cf, bool sync) {
    ColumnFamilyImpl *cfd = DCHECK_NOTNULL(ColumnFamilyHandle::Cast(cf)->impl());
    DCHECK_EQ(0, versions_->prev_log_number());
    Error rs = RenewLogger();
    if (!rs) {
        return rs;
    }
    VersionPatch patch;
    patch.SetRedoLogNumber(log_file_number_);
    rs = versions_->LogAndApply(options_, &patch, &mutex_);
    if (!rs) {
        return rs;
    }
    cfd->MakeImmutablePipeline(factory_.get(), log_file_number_);
    MaybeScheduleCompaction(cfd);
    
    std::unique_lock<std::mutex> lock(mutex_);
    if (sync && cfd->background_progress()) {
        cfd->mutable_background_cv()->wait(lock);
    }
    return Error::OK();
}

// REQUIRES: mutex_.lock()
Error DBImpl::RenewLogger() {
    if (log_file_) {
        log_file_->Flush();
        log_file_->Sync();
    }
    
    uint64_t new_log_number = versions_->GenerateFileNumber();
    std::string log_file_name = Files::LogFileName(abs_db_path_,
                                                   new_log_number);
    Error rs = env_->NewWritableFile(log_file_name, false, &log_file_);
    if (!rs) {
        versions_->ReuseFileNumber(new_log_number);
        return rs;
    }
    log_file_number_ = new_log_number;
    logger_.reset(new LogWriter(log_file_.get(), WAL::kDefaultBlockSize));
    return Error::OK();
}
    
Error DBImpl::Redo(uint64_t log_file_number,
                   core::SequenceNumber last_sequence_number,
                   core::SequenceNumber *update_sequence_number,
                   bool filter) {
    std::unique_ptr<SequentialFile> file;
    std::string log_file_name = Files::LogFileName(abs_db_path_, log_file_number);
    Error rs = env_->NewSequentialFile(log_file_name, &file,
                                       options_.allow_mmap_reads);
    if (!rs) {
        return rs;
    }
    LogReader logger(file.get(), true, WAL::kDefaultBlockSize);
    
    std::string_view result;
    std::string scatch;
    WritingHandler handler(log_file_number, filter, versions_->column_families());
    while (logger.Read(&result, &scatch)) {
        core::SequenceNumber sn = base::Slice::SetFixed64(result.substr(0, 8));
        uint32_t n_entries = base::Slice::SetFixed32(result.substr(8, 4));
        result.remove_prefix(WriteBatch::kHeaderSize);

        handler.ResetLastSequenceNumber(sn);
        rs = WriteBatch::Iterate(result.data(), result.size(), &handler);
        if (!rs) {
            return rs;
        }
        DCHECK_EQ(n_entries, handler.sequence_number_count());
        *update_sequence_number = sn + handler.sequence_number_count();
    }
    if (!logger.error().ok() && !logger.error().IsEof()) {
        return logger.error();
    }
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
    ctx->current = ctx->cfd->current();
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
    
    std::unique_lock<std::mutex> lock(mutex_);
    core::SequenceNumber last_sequence_number = versions_->last_sequence_number();
    Error rs = MakeRoomForWrite(cfd, &lock);
    if (!rs) {
        return rs;
    }
    
    std::string redo;
    MakeRedo(&redo, last_sequence_number, 1);
    core::KeyBoundle::MakeRedo(key, value, cfd->id(), flag, &redo);
    rs = logger_->Append(redo);
    if (!rs) {
        return rs;
    }
    flush_request_.fetch_add(1);
    if (opts.sync) {
        rs = logger_->Flush();
        if (!rs) {
            return rs;
        }
        rs = logger_->Sync(true);
        if (!rs) {
            return rs;
        }
    }
    
    // TODO: thiny locking
    cfd->mutable_table()->Put(key, value, last_sequence_number, flag);
    
    versions_->AddSequenceNumber(1);
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
    
Error DBImpl::GetTotalWalSize(uint64_t *size) {
    std::vector<std::string> children;
    Error rs = env_->GetChildren(abs_db_path_, &children);
    if (!rs) {
        return rs;
    }
    size_t new_total_size = 0;
    for (auto name : children) {
        Files::Kind kind;
        uint64_t number;
        std::tie(kind, number) = Files::ParseName(name);
        
        uint64_t file_size = 0;
        rs = env_->GetFileSize(abs_db_path_ + "/" + name, &file_size);
        if (!rs) {
            return rs;
        }
        new_total_size += file_size;
    }
    *size = new_total_size;
    return Error::OK();
}
    
// REQUIRES mutex_.lock()
Error DBImpl::MakeRoomForWrite(ColumnFamilyImpl *cfd,
                               std::unique_lock<std::mutex> *lock) {
    Error rs;
    if (bkg_error_.fail()) {
        return bkg_error_;
    }

    while (true) {
        
        if (cfd->background_error().fail()) {
            rs = cfd->background_error();
            //cfd->set_background_error(Error::OK());
            break;
        } else if (cfd->mutable_table()->ApproximateMemoryUsage() <
                   cfd->options().write_buffer_size &&
                   cfd->mutable_table()->ApproximateConflictFactor() <
                   cfd->options().conflict_factor_limit) {
            // Memory table usage samll than write buffer. Ignore it.
            // Memory table conflict-factor too small. Ignore it.
            break;
        } else if (cfd->immutable_pipeline()->InProgress()) {
            // Immutable table pipeline in progress.
            //cfd->mutable_background_cv()->wait(*lock);
            break;
        } else if (cfd->background_progress() &&
                   cfd->current()->level_files(0).size() >
                   Config::kMaxNumberLevel0File) {
            cfd->mutable_background_cv()->wait(*lock);
            break;
        } else {
            DCHECK_EQ(0, versions_->prev_log_number());
            rs = RenewLogger();
            if (!rs) {
                break;
            }
            VersionPatch patch;
            patch.SetRedoLogNumber(log_file_number_);
            rs = versions_->LogAndApply(options_, &patch, &mutex_);
            if (!rs) {
                break;
            }

            cfd->MakeImmutablePipeline(factory_.get(), log_file_number_);
            MaybeScheduleCompaction(cfd);
        }
        
        // TODO:
    }
    return rs;
}

void DBImpl::FlushWork() {
    DLOG(INFO) << "Flush thread start...";
    uint64_t last_sync_jiffy = env_->CurrentTimeMicros();
    
    std::unique_lock<std::mutex> lock(bkg_mutex_);
    while (!shutting_down_.load()) {
        bkg_cv_.wait_for(lock, std::chrono::milliseconds(200));
        
        int n_reqs = flush_request_.load();
        if (n_reqs > 0 &&
            (env_->CurrentTimeMicros() - last_sync_jiffy) / 1000 >
            Config::kMaxWalSyncMills) {

            mutex_.lock();
            bkg_error_ = log_file_->Flush();
            if (bkg_error_.ok()) {
                bkg_error_ = log_file_->Sync();
            }
            if (bkg_error_.fail()) {
                mutex_.unlock();
                continue;
            }
            mutex_.unlock();
            DLOG(INFO) << "Flush ok.";
            last_sync_jiffy = env_->CurrentTimeMicros();
            flush_request_.store(0);
        }
        
        // Compute all wal log file size
        uint64_t wal_size = 0;
        bkg_error_ = GetTotalWalSize(&wal_size);
        if (bkg_error_.fail()) {
            continue;
        }
        total_wal_size_.store(wal_size, std::memory_order_release);
    }
    
    DLOG(INFO) << "Flush thread stopped...";
}

// REQUIRES mutex_.lock()
void DBImpl::MaybeScheduleCompaction(ColumnFamilyImpl *cfd) {
    if (bkg_active_.load() > 0) {
        return; // Compaction is running.
    }
    
    if (shutting_down_.load()) {
        return; // Is shutting down, ignore schedule
    }
    
    if (!cfd->immutable_pipeline()->InProgress() &&
        !cfd->NeedsCompaction()) {
        return; // Compaction is no need
    }
    
    bkg_active_.fetch_add(1);
    cfd->set_background_progress(true);
    std::thread([this](auto cfd) {
        this->BackgroundWork(cfd);
    }, cfd).detach();
}
    
void DBImpl::BackgroundWork(ColumnFamilyImpl *cfd) {
    DLOG(INFO) << "Background work on. " << cfd->name();
    
    DCHECK_GT(bkg_active_.load(), 0);
    if (!shutting_down_.load()) {
        std::unique_lock<std::mutex> lock(mutex_);
        BackgroundCompaction(cfd);
    }
    bkg_active_.fetch_sub(1);
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

    uint64_t wal_size = 0;
    bkg_error_ = GetTotalWalSize(&wal_size);
    if (bkg_error_.ok()) {
        total_wal_size_.store(wal_size, std::memory_order_release);
    }
}

// REQUIRES mutex_.lock()
Error DBImpl::CompactMemoryTable(ColumnFamilyImpl *cfd) {
    DCHECK(cfd->immutable_pipeline()->InProgress());
    
    Error rs;
    VersionPatch patch;
    base::intrusive_ptr<core::MemoryTable> imm;
    while (cfd->immutable_pipeline()->Peek(&imm)) {
        DCHECK(!imm.is_null());
        rs = WriteLevel0Table(cfd->current(), &patch, imm.get());
        if (!rs) {
            return rs;
        }
        if (shutting_down_.load()) {
            return MAI_IO_ERROR("Deleting DB during memtable compaction");
        }
        patch.SetPrevLogNumber(0);
        patch.SetRedoLog(cfd->id(), imm->associated_file_number());
        rs = versions_->LogAndApply(ColumnFamilyOptions{}, &patch, &mutex_);
        if (!rs) {
            return rs;
        }

        cfd->immutable_pipeline()->Take(&imm);
        patch.Reset();
    }
    return Error::OK();
}

// REQUIRES mutex_.lock()
Error DBImpl::CompactFileTable(ColumnFamilyImpl *cfd, CompactionContext *ctx) {
    DCHECK_GE(ctx->level, 0);
    DCHECK_LT(ctx->level, Config::kMaxLevel - 1);
    
    base::intrusive_ptr<table::TablePropsBoundle> boundle;
    size_t n_entries = 0;
    for (auto fmd : ctx->inputs[0]) {
        Error rs = table_cache_->GetTableProperties(cfd, fmd->number, &boundle);
        if (!rs) {
            return rs;
        }
        n_entries += boundle->data().num_entries;
    }
    for (auto fmd : ctx->inputs[1]) {
        Error rs = table_cache_->GetTableProperties(cfd, fmd->number, &boundle);
        if (!rs) {
            return rs;
        }
        n_entries += boundle->data().num_entries;
    }
    size_t new_num_slots = Config::ComputeNumSlots(ctx->level + 1, n_entries,
                                                   Config::kLimitMinNumberSlots);
    
    std::unique_ptr<Compaction>
    job(factory_->NewCompaction(abs_db_path_, cfd->ikcmp(),
                                table_cache_.get(), cfd));
    job->set_target_level(ctx->level + 1);
    job->set_compaction_point(cfd->compaction_point(ctx->level));
    job->set_input_version(ctx->input_version);
    job->set_target_file_number(versions_->GenerateFileNumber());
    
    if (snapshots_.empty()) {
        job->set_smallest_snapshot(versions_->last_sequence_number());
    } else {
        job->set_smallest_snapshot(snapshots_.oldest()->sequence_number());
    }
    
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
        versions_->ReuseFileNumber(job->target_file_number());
        return rs;
    }
    std::unique_ptr<table::TableBuilder>
    builder(factory_->NewTableBuilder(cfd->options().use_unordered_table ?
                                      "s1t" : "sst",
                                      cfd->ikcmp(),
                                      file.get(),
                                      cfd->options().block_size,
                                      cfd->options().block_restart_interval,
                                      new_num_slots,
                                      n_entries));
    CompactionResult result;
    mutex_.unlock();
    rs = job->Run(builder.get(), &result); // FIXME:
    mutex_.lock();
    if (!rs) {
        builder->Abandon();
        return rs;
    }
    DCHECK_GT(builder->NumEntries(), 0) << "Empty compaction!";
    
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
    
    std::unique_ptr<Iterator> iter(table->NewIterator());
    if (iter->error().fail()) {
        return iter->error();
    }
    uint64_t file_number = versions_->GenerateFileNumber();
    LOG(INFO) << "Level0 table compaction start, target file number: "
        << file_number;
    mutex_.unlock(); // Do not need DB lock -------------------------------------
    
    uint64_t jiffies = env_->CurrentTimeMicros();
    
    std::unique_ptr<WritableFile> file;
    std::string table_file_name = current->owns()->GetTableFileName(file_number);
    Error rs = env_->NewWritableFile(table_file_name, false, &file);
    if (!rs) {
        versions_->ReuseFileNumber(file_number);
        mutex_.lock();
        return rs;
    }
    
    ColumnFamilyImpl *cfd = current->owns();
    size_t new_num_slots = Config::ComputeNumSlots(0, table->NumEntries(),
                                                   Config::kLimitMinNumberSlots);

    std::unique_ptr<table::TableBuilder>
        builder(factory_->NewTableBuilder(cfd->options().use_unordered_table ?
                                          "s1t" : "sst",
                                          cfd->ikcmp(),
                                          file.get(), cfd->options().block_size,
                                          cfd->options().block_restart_interval,
                                          new_num_slots,
                                          table->NumEntries()));
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
    
    FileMetaData *fmd = new FileMetaData(file_number);
    fmd->ctime        = env_->CurrentTimeMicros();
    fmd->size         = builder->FileSize();
    fmd->largest_key  = largest_key;
    fmd->smallest_key = smallest_key;
    patch->CreaetFile(cfd->id(), 0, fmd);
    
    DLOG(INFO) << "Cost: " << (env_->CurrentTimeMicros() - jiffies) / 1000.0 << " ms "
               << "[" << core::KeyBoundle::ExtractUserKey(fmd->smallest_key)
               << "," << core::KeyBoundle::ExtractUserKey(fmd->largest_key)
               << "]";
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
    cleanup.erase(versions_->manifest_file_number());
    
    for (ColumnFamilyImpl *cfd : *versions_->column_families()) {
        cleanup.erase(cfd->redo_log_number());
    }
    
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
    
/*static*/
Error DB::Open(const Options &opts, const std::string &name,
               const std::vector<ColumnFamilyDescriptor> &descriptors,
               std::vector<ColumnFamily *> *column_families, DB **result) {
    if (name.empty()) {
        return MAI_CORRUPTION("Empty db name.");
    }
    db::DBImpl *impl = new db::DBImpl(name, opts);
    Error rs = impl->Open(descriptors, column_families);
    if (!rs) {
        return rs;
    }
    
    *result = impl;
    return Error::OK();
}

/*static*/
Error DB::ListColumnFamilies(const Options &opts, const std::string &name,
                             std::vector<std::string> *result) {
    Env *env = opts.env;

    std::string abs_db_path = env->GetAbsolutePath(name);;
    std::string current_file_name = db::Files::CurrentFileName(abs_db_path);
    
    Error rs = env->FileExists(current_file_name);
    if (!rs) {
        return rs;
    }
    
    std::string number;
    rs = base::FileReader::ReadAll(current_file_name, &number, env);
    if (!rs) {
        return rs;
    }
    
    uint64_t mn = 0;
    if (base::Slice::ParseU64(number.data(), number.size(), &mn) != 0) {
        return MAI_IO_ERROR("Bad manifest number! " + number);
    }
    
    std::string manifest_file_name = db::Files::ManifestFileName(abs_db_path, mn);
    std::unique_ptr<SequentialFile> file;
    rs = env->NewSequentialFile(manifest_file_name, &file, false);
    if (!rs) {
        return rs;
    }
    
    std::map<uint32_t, std::string> cfs;
    
    db::VersionPatch patch;
    std::string_view record;
    std::string scratch;
    db::LogReader rd(file.get(), true, db::WAL::kDefaultBlockSize);
    while (rd.Read(&record, &scratch)) {
        patch.Reset();
        patch.Decode(record);
        
        if (patch.has_add_column_family()) {
            cfs.insert({patch.cf_creation().cfid, patch.cf_creation().name});
        }
        if (patch.has_drop_column_family()) {
            cfs.erase(patch.cf_deletion());
        }
    }
    for (const auto &cf : cfs) {
        result->push_back(cf.second);
    }
    return Error::OK();
}
    
} // namespace mai
