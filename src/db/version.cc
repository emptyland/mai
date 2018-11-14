#include "db/version.h"
#include "db/column-family.h"
#include "db/files.h"
#include "db/write-ahead-log.h"

namespace mai {
    
namespace db {
    
VersionSet::VersionSet(const std::string db_name, const Options &options)
    : db_name_(db_name)
    , env_(DCHECK_NOTNULL(options.env))
    , column_families_(new ColumnFamilySet(db_name))
    , ikcmp_(DCHECK_NOTNULL(options.comparator)) {
}

VersionSet::~VersionSet() {    
}
    
Error VersionSet::LogAndApply(VersionPatch *patch, std::mutex *mutex) {
    if (patch->has_field(VersionPatch::kRedoLogNumber)) {
        
    }
    
    return MAI_NOT_SUPPORTED("TODO:");
}
    
Error VersionSet::CreateManifestFile() {
    
    return MAI_NOT_SUPPORTED("TODO:");
}
    
} // namespace db
    
} // namespace mai
