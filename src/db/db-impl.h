#ifndef MAI_DB_DB_IMPL_H_
#define MAI_DB_DB_IMPL_H_

#include "base/base.h"
#include "mai/db.h"
#include "mai/options.h"
#include "mai/write-batch.h"

namespace mai {
    
namespace db {

class DBImpl final : public DB {
public:
    DBImpl();
    virtual ~DBImpl();
    
    virtual Error
    NewColumnFamilies(const std::vector<std::string> &names,
                      const ColumnFamilyOptions &options,
                      std::vector<ColumnFamily *> *result) override;
    virtual Error
    DropColumnFamilies(const std::vector<ColumnFamily *> &column_families) override;
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DBImpl);
private:
    
}; // class DBImpl

} // namespace db

    
} // namespace mai

#endif // MAI_DB_DB_IMPL_H_
