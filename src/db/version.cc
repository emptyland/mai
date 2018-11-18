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
    , ikcmp_(DCHECK_NOTNULL(options.comparator))
    , block_size_(options.block_size) {
}

VersionSet::~VersionSet() {    
}
    
Error VersionSet::LogAndApply(VersionPatch *patch, std::mutex *mutex) {
    if (patch->has_field(VersionPatch::kRedoLogNumber)) {
        
    }
    
    return MAI_NOT_SUPPORTED("TODO:");
}
    
Error VersionSet::CreateManifestFile() {
    DCHECK(!log_file_);
    
    manifest_file_number_ = GenerateFileNumber();
    std::string file_name = Files::ManifestFileName(db_name_,
                                                    manifest_file_number_);
    Error rs = env_->NewWritableFile(file_name, &log_file_);
    if (!rs) {
        return rs;
    }
    logger_.reset(new LogWriter(log_file_.get(), block_size_));
    
    return WriteCurrentSnapshot();
}
    
Error VersionSet::WriteCurrentSnapshot() {
    VersionPatch patch;
    
    for (ColumnFamilyImpl *cf : *column_families_) {
        patch.Reset();
        patch.AddColumnFamily(cf->name(), cf->id(), cf->comparator()->Name());
    }
    
    patch.Reset();
    patch.set_max_column_faimly(column_families_->max_column_family());
    patch.set_last_sequence_number(last_sequence_number_);
    patch.set_next_file_number(next_file_number_);
    patch.set_prev_log_number(prev_log_number_);
    patch.set_redo_log_number(redo_log_number_);
    

    return MAI_NOT_SUPPORTED("TODO:");
}
    
Error VersionSet::LogPatch(const VersionPatch &patch) {
    return MAI_NOT_SUPPORTED("TODO:");
}
    
} // namespace db
    
} // namespace mai
