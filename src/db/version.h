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
class WritableFile;
struct Options;
namespace db {
    
class ColumnFamilySet;
struct FileMetadata;
class VersionPatch;
class Version;
class VersionSet;
class LogWriter;
    
static const int kMaxLevel = 4;
    
struct FileMetadata final : public base::ReferenceCounted<FileMetadata> {
    
    uint64_t number;
    
    std::string smallest_key;
    std::string largest_key;
    
    uint64_t size = 0;
    uint64_t ctime = 0;
    
    FileMetadata(uint64_t file_number) : number(file_number) {}
}; // struct FileMetadata

class VersionPatch final {
public:
    enum Field {
        kComparator,
        kLastSequenceNumber,
        kNextFileNumber,
        kRedoLogNumber,
        kPrevLogNumber,
        kCompactionPoint,
        kDeletion,
        kCreation,
        kSetMaxColumnFamily,
        kAddCloumnFamily,
        kDropCloumnFamily,
        kMaxFields,
    };
    
    VersionPatch() { Reset(); }
    ~VersionPatch() {}
    
    void set_comparator(uint32_t cfid, const std::string &name) {
        set_field(kComparator);
        column_family_id_ = cfid;
        comparator_.assign(name);
    }
    
    void set_last_sequence_number(core::SequenceNumber version) {
        set_field(kLastSequenceNumber);
        last_sequence_number_ = version;
    }
    
    void set_next_file_number(uint64_t number) {
        set_field(kNextFileNumber);
        next_file_number_ = number;
    }
    
    void set_redo_log_number(uint64_t number) {
        set_field(kRedoLogNumber);
        redo_log_number_ = number;
    }
    
    void set_prev_log_number(uint64_t number) {
        set_field(kPrevLogNumber);
        prev_log_number_ = number;
    }
    
    void set_max_column_faimly(uint32_t max_column_family) {
        set_field(kSetMaxColumnFamily);
        max_column_family_ = max_column_family;
    }
    
    void DeleteFile(uint32_t cfid, int level, uint64_t number) {
        set_field(kDeletion);
        deletion_.push_back({cfid, level, number});
    }
    
    void CreaetFile(uint32_t cfid, int level, FileMetadata *fmd) {
        set_field(kCreation);
        creation_.push_back({cfid, level, base::MakeRef(fmd)});
    }
    
    void DropColumnFamily(const uint32_t cfid) {
        set_field(kDropCloumnFamily);
        column_family_id_ = cfid;
    }
    
    void AddColumnFamily(const std::string name, uint32_t cfid,
                         const std::string comparator_name) {
        set_field(kAddCloumnFamily);
        column_family_id_ = cfid;
        column_family_name_.assign(name);
        comparator_.assign(comparator_name);
    }
    
    void Encode(std::string *buf) const;
    void Decode(const std::string_view buf);
    
    bool has_field(Field field) const {
        int i = static_cast<int>(field);
        DCHECK_GE(i, 0); DCHECK_LT(i, kMaxFields);
        return fields_[i / 32] & (1 << (i % 32));
    }
    
    void Reset() { ::memset(fields_, 0, arraysize(fields_) * sizeof(uint32_t)); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(VersionPatch);
private:
    struct Deletion {
        uint32_t cfid;
        int      level;
        uint64_t number;
    };
    
    struct Creation {
        uint32_t cfid;
        int      level;
        base::Handle<FileMetadata> file_metadata;
    };
    
    typedef std::vector<Creation> CreationCollection;
    typedef std::vector<Deletion> DeletionCollection;

    void set_field(Field field) {
        int i = static_cast<int>(field);
        DCHECK_GE(i, 0); DCHECK_LT(i, kMaxFields);
        fields_[i / 32] |= (1 << (i % 32));
    }
    
    uint32_t column_family_id_;
    std::string column_family_name_;
    uint32_t max_column_family_;
    
    std::string comparator_;
    
    core::SequenceNumber last_sequence_number_;
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
    explicit Version(VersionSet *owner) : owner_(owner) {}
    
    DEF_PTR_GETTER_NOTNULL(VersionSet, owner);
    DEF_PTR_GETTER(Version, next);
    DEF_PTR_GETTER(Version, prev);
    
    const std::vector<base::Handle<FileMetadata>> &level_files(int level) {
        DCHECK_GE(level, 0);
        DCHECK_LT(level, kMaxLevel);
        return files_[level];
    }
    
    friend class ColumnFamilyImpl;
    friend class VersionSet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Version);
private:
    VersionSet *owner_;
    Version *next_ = nullptr;
    Version *prev_ = nullptr;
    std::vector<base::Handle<FileMetadata>> files_[kMaxLevel];
}; // class Version
    

class VersionSet final {
public:
    VersionSet(const std::string db_name, const Options &options);
    ~VersionSet();
    
    uint64_t GenerateFileNumber() { return next_file_number_++; }
    
    Error LogAndApply(VersionPatch *patch, std::mutex *mutex);
    
    Error CreateManifestFile();
    Error WriteCurrentSnapshot();
    
    ColumnFamilySet *column_families() const { return column_families_.get(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(VersionSet);
private:
    Error LogPatch(const VersionPatch &patch);
    
    const std::string db_name_;
    Env *const env_;
    std::unique_ptr<ColumnFamilySet> column_families_;
    const core::InternalKeyComparator ikcmp_;
    const uint64_t block_size_;

    core::SequenceNumber last_sequence_number_ = 0;
    
    uint64_t next_file_number_ = 0;
    uint64_t redo_log_number_ = 0;
    uint64_t prev_log_number_ = 0;
    uint64_t manifest_file_number_ = 0;
    
    std::unique_ptr<WritableFile> log_file_;
    std::unique_ptr<LogWriter> logger_;
}; // class VersionSet

} // namespace db
    
} // namespace mai


#endif // MAI_DB_VERSION_H_
