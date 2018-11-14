#ifndef MAI_DB_H_
#define MAI_DB_H_

#include "mai/options.h"
#include "mai/error.h"
#include <string>
#include <string_view>
#include <vector>


namespace mai {
    
class Comparator;
class Iterator;
class WriteBatch;
    
extern const char kDefaultColumnFamilyName[];
    
struct ColumnFamilyDescriptor {
    std::string         name = kDefaultColumnFamilyName;
    ColumnFamilyOptions options;
}; // struct ColumnFamilyDescriptor

    
class ColumnFamily {
public:
    ColumnFamily() {}
    virtual ~ColumnFamily() {}
    virtual std::string name() = 0;
    virtual uint32_t id() = 0;
    virtual const Comparator *comparator() = 0;
    virtual Error GetDescriptor(ColumnFamilyDescriptor *desc) = 0;

    ColumnFamily(const ColumnFamily &) = delete;
    ColumnFamily(ColumnFamily &&) = delete;
    void operator = (const ColumnFamily &) = delete;
}; // class ColumnFamily
    
class Snapshot {
public:
    Snapshot() {}
    virtual ~Snapshot() {}
    
    Snapshot(const Snapshot &) = delete;
    Snapshot(Snapshot &&) = delete;
    void operator = (const Snapshot &) = delete;
}; // class Snapshot
    
class DB {
public:
    DB() {}
    virtual ~DB() {}
    
    static Error Open(const Options &opts,
                      const std::string name,
                      const std::vector<ColumnFamilyDescriptor> &descriptors,
                      std::vector<ColumnFamily *> *column_families,
                      DB **result);
    
    virtual Error NewColumnFamilies(const std::vector<std::string> &names,
                                    const ColumnFamilyOptions &options,
                                    std::vector<ColumnFamily *> *result) = 0;
    
    virtual Error
    DropColumnFamilies(const std::vector<ColumnFamily *> &column_families) = 0;
    
    virtual Error GetAllColumnFamilies(std::vector<ColumnFamily *> *result) = 0;
    
    virtual Error Put(const WriteOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string_view value) = 0;
    
    virtual Error Delete(const WriteOptions &opts, ColumnFamily *cf,
                         std::string_view key) = 0;
    
    virtual Error Write(const WriteOptions& opts, WriteBatch* updates) = 0;
    
    virtual Error Get(const ReadOptions &opts, ColumnFamily *cf,
                      std::string_view key, std::string *value) = 0;
    
    virtual Iterator *NewIterator(const ReadOptions &opts, ColumnFamily *cf) = 0;
    
    virtual const Snapshot *GetSnapshot() = 0;
    
    virtual void ReleaseSnapshot(const Snapshot *snapshot) = 0;
    
    DB(const DB &) = delete;
    DB(DB &&) = delete;
    void operator = (const DB &) = delete;
}; // class DB
    
} // namespace mai

#endif // MAI_DB_H_
