#include "db/db-impl.h"
#include "db/write-ahead-log.h"
#include "core/key-boundle.h"
#include "mai/env.h"
#include "glog/logging.h"

namespace mai {
    
namespace db {
    
    
DBImpl::DBImpl(const std::string &db_name, Env *env)
    : db_name_(db_name)
    , env_(DCHECK_NOTNULL(env)) {}

DBImpl::~DBImpl() {
    
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
    return MAI_NOT_SUPPORTED("TODO:");
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
    
} // namespace db    
    
} // namespace mai
