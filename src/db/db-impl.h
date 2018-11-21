#ifndef MAI_DB_DB_IMPL_H_
#define MAI_DB_DB_IMPL_H_

#include "base/base.h"
#include "mai/db.h"
#include "mai/options.h"
#include "mai/write-batch.h"
#include "db/snapshot-impl.h"
#include <mutex>

namespace mai {
class WritableFile;
class Env;
namespace db {
class LogWriter;
class TableCache;
class VersionSet;
class Factory;
class ColumnFamilyImpl;
    
struct GetContext;

class DBImpl final : public DB {
public:
    DBImpl(const std::string &db_name, Env *env);
    virtual ~DBImpl();
    
    Error Open(const Options &opts,
               const std::vector<ColumnFamilyDescriptor> &descriptors,
               std::vector<ColumnFamily *> *column_families);
    
    Error NewDB(const Options &opts,
                const std::vector<ColumnFamilyDescriptor> &desc);
    
    Error Recovery(const Options &opts,
                   const std::vector<ColumnFamilyDescriptor> &desc);
    
    virtual Error
    NewColumnFamilies(const std::vector<std::string> &names,
                      const ColumnFamilyOptions &options,
                      std::vector<ColumnFamily *> *result) override;
    virtual Error
    DropColumnFamilies(const std::vector<ColumnFamily *> &column_families) override;
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DBImpl);
private:
    Error PrepareForGet(const ReadOptions &opts, ColumnFamily *cf,
                        GetContext *ctx);
    Error Write(const WriteOptions &opts, ColumnFamily *cf,
                std::string_view key, std::string_view value, uint8_t flag);
    Error MakeRoomForWrite(ColumnFamilyImpl *cf, bool force);
    
    const std::string db_name_;
    Env *const env_;
    
    SnapshotList snapshots_;
    std::unique_ptr<ColumnFamily> default_cf_;
    std::unique_ptr<Factory> factory_;
    std::unique_ptr<VersionSet> versions_;
    std::unique_ptr<TableCache> table_cache_;
    std::unique_ptr<WritableFile> log_file_;
    std::unique_ptr<LogWriter> logger_;
    uint64_t log_file_number_ = 0;
    std::mutex mutex_; // DB lock
}; // class DBImpl

} // namespace db

    
} // namespace mai

#endif // MAI_DB_DB_IMPL_H_
