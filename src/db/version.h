#ifndef MAI_DB_VERSION_H_
#define MAI_DB_VERSION_H_

#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/reference-count.h"
#include "base/base.h"
#include "mai/error.h"
#include <vector>
#include <set>
#include <mutex>

namespace mai {
class Env;
class SequentialFile;
struct Options;
namespace db {
    
class ColumnFamilySet;
struct FileMetadata;
class VersionPatch;
class Version;
class VersionSet;
class LogWriter;
    
struct FileMetadata final : public base::ReferenceCounted<FileMetadata> {
    
    uint64_t number;
    
    std::string smallest_key;
    std::string largest_key;
    
    uint64_t size = 0;
    uint64_t ctime = 0;
    
    FileMetadata(uint64_t file_number) : number(file_number) {}
}; // struct FileMetadata

class VersionPatch final : public base::ReferenceCounted<VersionPatch> {
public:
    enum Field {
        kComparator,
        kLastVersion,
        kNextFileNumber,
        kRedoLogNumber,
        kPrevLogNumber,
        kCompactionPoint,
        kDeletion,
        kCreation,
        kAddCloumnFamily,
        kDropCloumnFamily,
        kMaxFields,
    };
    
    VersionPatch() {}
    ~VersionPatch() {}
    
    void Encode(std::string *buf) const;
    void Decode(const std::string_view buf);
    
    bool has_field(Field field) const {
        int i = static_cast<int>(field);
        DCHECK_GE(i, 0); DCHECK_LT(i, kMaxFields);
        return fields_[i / 32] & (1 << (i % 32));
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(VersionPatch);
private:
    typedef std::vector<std::pair<int, base::Handle<FileMetadata>>>
        CreationCollection;
    
    typedef std::set<std::pair<int, uint64_t>> DeletionCollection;

    void set_field(Field field) {
        int i = static_cast<int>(field);
        DCHECK_GE(i, 0); DCHECK_LT(i, kMaxFields);
        fields_[i / 32] |= (1 << (i % 32));
    }
    
    uint32_t column_family_id_;
    std::string column_family_name_;
    
    std::string comparator_;
    
    core::SequenceNumber last_version_;
    uint64_t next_file_number_;
    uint64_t redo_log_number_;
    uint64_t prev_log_number_;
    
    int compaction_level_;
    std::string compaction_key_;
    
    CreationCollection creation_;
    DeletionCollection deletion_;
    
    uint32_t fields_[(kMaxFields + 31) / 32];
}; // class VersionPatch
    
    
class Version final {
public:
    
    DEF_PTR_GETTER(Version, next);
    DEF_PTR_GETTER(Version, prev);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Version);
private:
    Version *next_ = nullptr;
    Version *prev_ = nullptr;
}; // class Version
    

class VersionSet final {
public:
    VersionSet(const std::string db_name, const Options &options);
    ~VersionSet();
    
    uint64_t GenerateFileNumber() { return next_file_number_++; }
    
    Error LogAndApply(VersionPatch *patch, std::mutex *mutex);
    
    Error CreateManifestFile();
    
    ColumnFamilySet *column_families() const { return column_families_.get(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(VersionSet);
private:
    const std::string db_name_;
    Env *const env_;
    std::unique_ptr<ColumnFamilySet> column_families_;
    const core::InternalKeyComparator ikcmp_;

    core::SequenceNumber last_version_ = 0;
    
    uint64_t next_file_number_ = 0;
    uint64_t redo_log_number_ = 0;
    uint64_t prev_log_number_ = 0;
    uint64_t manifest_file_number_ = 0;
    
    std::unique_ptr<SequentialFile> log_file_;
    std::unique_ptr<LogWriter> logger_;
}; // class VersionSet

} // namespace db
    
} // namespace mai


#endif // MAI_DB_VERSION_H_
