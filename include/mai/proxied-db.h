#ifndef MAI_PROXIED_DB_H_
#define MAI_PROXIED_DB_H_

#include "mai/db.h"

namespace mai {
    
class ProxiedDB : public DB {
public:
    explicit ProxiedDB(DB *db) : db_(db) {}

    virtual ~ProxiedDB() override {
        delete db_;
        db_ = nullptr;
    }
    
    virtual Error
    NewColumnFamilies(const std::vector<std::string> &names,
                      const ColumnFamilyOptions &options,
                      std::vector<ColumnFamily *> *result) override {
        return db_->NewColumnFamilies(names, options, result);
    }
    virtual Error NewColumnFamily(const std::string &name,
                                  const ColumnFamilyOptions &options,
                                  ColumnFamily **result) override {
        return db_->NewColumnFamily(name, options, result);
    }
    virtual Error
    DropColumnFamilies(const std::vector<ColumnFamily *> &column_families) override {
        return db_->DropColumnFamilies(column_families);
    }
    virtual Error DropColumnFamily(ColumnFamily *cf) override {
        return db_->DropColumnFamily(cf);
    }
    virtual Error ReleaseColumnFamily(ColumnFamily *cf) override {
        return db_->ReleaseColumnFamily(cf);
    }
    virtual Error
    GetAllColumnFamilies(std::vector<ColumnFamily *> *result) override {
        return db_->GetAllColumnFamilies(result);
    }
    virtual Error Put(const WriteOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string_view value) override {
        return db_->Put(opts, cf, key, value);
    }
    virtual Error Delete(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key) override {
        return db_->Delete(opts, cf, key);
    }
    virtual Error Write(const WriteOptions& opts, WriteBatch* updates) override {
        return db_->Write(opts, updates);
    }
    virtual Error Get(const ReadOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string *value) override {
        return db_->Get(opts, cf, key, value);
    }
    virtual Iterator *
    NewIterator(const ReadOptions &opts, ColumnFamily *cf) override {
        return db_->NewIterator(opts, cf);
    }
    virtual const Snapshot *GetSnapshot() override {
        return db_->GetSnapshot();
    }
    virtual void ReleaseSnapshot(const Snapshot *snapshot) override {
        return db_->ReleaseSnapshot(snapshot);
    }
    virtual ColumnFamily *DefaultColumnFamily() override {
        return db_->DefaultColumnFamily();
    }
    virtual Error
    GetProperty(std::string_view property, std::string *value) override {
        return db_->GetProperty(property, value);
    }
    
    DB *GetDB() const { return db_; }
    
    ProxiedDB(const ProxiedDB &) = delete;
    ProxiedDB(ProxiedDB &&) = delete;
    void operator = (const ProxiedDB &) = delete;
private:
    DB *db_;
};
    
} // namespace mai


#endif // MAI_PROXIED_DB_H_
