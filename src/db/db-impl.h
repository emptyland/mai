#ifndef MAI_DB_DB_IMPL_H_
#define MAI_DB_DB_IMPL_H_

#include "db/snapshot-impl.h"
#include "base/reference-count.h"
#include "base/base.h"
#include "mai/db.h"
#include "mai/options.h"
#include "mai/write-batch.h"
#include <thread>
#include <mutex>

namespace mai {
class WritableFile;
class Env;
namespace core {
class MemoryTable;
} // namespace core
namespace table {
class BlockCache;
} // namespace table
namespace db {
class LogWriter;
class TableCache;
class VersionSet;
class Version;
class VersionPatch;
class Factory;
class ColumnFamilyImpl;
struct CompactionContext;
class DBImpl;
struct GetContext;
    
class WriteCallback {
public:
    virtual Error Prepare(DBImpl *db) = 0;
    virtual void WALDone(DBImpl *db) {}
    virtual void Done(DBImpl *db) {}
}; // class WriteCallback

class DBImpl final : public DB {
public:
    DBImpl(const std::string &db_name, const Options &opts);
    virtual ~DBImpl();
    
    DEF_VAL_GETTER(std::string, db_name);
    DEF_VAL_GETTER(std::string, abs_db_path);
    DEF_PTR_GETTER_NOTNULL(Env, env);
    
    Error Open(const std::vector<ColumnFamilyDescriptor> &descriptors,
               std::vector<ColumnFamily *> *column_families);
    
    Error NewDB(const std::vector<ColumnFamilyDescriptor> &desc);
    
    Error Recovery(const std::vector<ColumnFamilyDescriptor> &desc);

    virtual Error NewColumnFamily(const std::string &name,
                                  const ColumnFamilyOptions &options,
                                  ColumnFamily **result) override;
    virtual Error DropColumnFamily(ColumnFamily *column_family) override;
    virtual Error ReleaseColumnFamily(ColumnFamily *column_family) override;
    virtual Error
    GetAllColumnFamilies(std::vector<ColumnFamily *> *result) override;
    virtual Error Put(const WriteOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string_view value) override;
    virtual Error Delete(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key) override;
    virtual Error Write(const WriteOptions& opts, WriteBatch* updates) override;
    virtual Error Get(const ReadOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string *value) override;
    virtual Iterator *
    NewIterator(const ReadOptions &opts, ColumnFamily *cf) override;
    virtual const Snapshot *GetSnapshot() override;
    virtual void ReleaseSnapshot(const Snapshot *snapshot) override;
    virtual ColumnFamily *DefaultColumnFamily() override;
    virtual Error GetProperty(std::string_view property,
                              std::string *value) override;
    
    Error WriteImpl(const WriteOptions& opts, WriteBatch* batch,
                    WriteCallback *callback);
    Iterator *NewInternalIterator(const ReadOptions &opts, ColumnFamilyImpl *cfd);
    core::SequenceNumber GetLatestSequenceNumber();

    Error GetColumnFamilyImpl(uint32_t cfid,
                              base::intrusive_ptr<ColumnFamilyImpl> *result);
    Error  GetLatestSequenceForKey(ColumnFamilyImpl *impl,
                                   bool cache_only,
                                   std::string_view key,
                                   core::SequenceNumber *seq);

    //void TEST_PrintFiles(ColumnFamily *cf);
    Error TEST_ForceDumpImmutableTable(ColumnFamily *cf, bool sync);
    TableCache *TEST_GetTableCache() { return table_cache_.get(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DBImpl);
private:
    Error RenewLogger();
    Error Redo(uint64_t log_file_number,
               core::SequenceNumber last_sequence_number,
               core::SequenceNumber *update_sequence_number,
               bool filter);
    Error PrepareForGet(const ReadOptions &opts, ColumnFamily *cf,
                        GetContext *ctx);
    Error Write(const WriteOptions &opts, ColumnFamily *cf,
                std::string_view key, std::string_view value, uint8_t flag);
    Error MakeRoomForWrite(ColumnFamilyImpl *cfd,
                           std::unique_lock<std::mutex> *lock);
    void MaybeScheduleCompaction(ColumnFamilyImpl *cfd);
    void BackgroundWork(ColumnFamilyImpl *cfd);
    void BackgroundCompaction(ColumnFamilyImpl *cfd);
    void FlushWork();
    Error CompactMemoryTable(ColumnFamilyImpl *cfd);
    Error CompactFileTable(ColumnFamilyImpl *cfd, CompactionContext *ctx);
    Error WriteLevel0Table(Version *current, VersionPatch *patch,
                           core::MemoryTable *imm);
    void DeleteObsoleteFiles(ColumnFamilyImpl *cfd);
    Error InternalNewColumnFamily(const std::string &name,
                                  const ColumnFamilyOptions &opts,
                                  uint32_t *cfid);
    Error GetTotalWalSize(uint64_t *size);
    
    const std::string db_name_;
    const Options options_;
    Env *const env_;
    const std::string abs_db_path_;
    std::unique_ptr<Factory> factory_;
    std::atomic<int> bkg_active_;
    std::atomic<bool> shutting_down_;
    //std::unique_ptr<table::BlockCache> block_cache_;
    std::unique_ptr<TableCache> table_cache_;
    std::unique_ptr<VersionSet> versions_;
    std::atomic<uint64_t> total_wal_size_;
    
    SnapshotList snapshots_;
    std::unique_ptr<ColumnFamily> default_cf_;
    std::unique_ptr<WritableFile> log_file_;
    std::unique_ptr<LogWriter> logger_;
    uint64_t log_file_number_ = 0;
    std::atomic<int> flush_request_;
    std::thread flush_worker_;
    Error bkg_error_;
    std::condition_variable bkg_cv_;
    std::mutex bkg_mutex_; // Only for bkg_cv_
    std::mutex mutex_; // DB lock
}; // class DBImpl

} // namespace db

    
} // namespace mai

#endif // MAI_DB_DB_IMPL_H_
