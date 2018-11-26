#ifndef MAI_DB_VERSION_H_
#define MAI_DB_VERSION_H_

#include "db/config.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/reference-count.h"
#include "base/base.h"
#include "mai/error.h"
#include "mai/options.h"
#include <vector>
#include <set>
#include <mutex>
#include <map>
#include <numeric>

namespace mai {
class Env;
class SequentialFile;
class WritableFile;
namespace db {
    
struct FileMetaData;
class ColumnFamilySet;
class ColumnFamilyImpl;
class VersionBuilder;
class VersionPatch;
class VersionSet;
class Version;
class TableCache;
class LogWriter;
    
struct FileMetaData final : public base::ReferenceCounted<FileMetaData> {
    
    uint64_t number;
    
    std::string smallest_key;
    std::string largest_key;
    
    uint64_t size = 0;
    uint64_t ctime = 0;
    
    FileMetaData(uint64_t file_number) : number(file_number) {}
}; // struct FileMetadata
    
#define VERSION_FIELDS(V) \
    V(LastSequenceNumber, last_sequence_number) \
    V(NextFileNumber, next_file_number) \
    V(RedoLog, redo_log) \
    V(PrevLogNumber, prev_log_number) \
    V(CompactionPoint, compaction_point) \
    V(Deletion, deletion) \
    V(Creation, creation) \
    V(MaxColumnFamily, max_column_family) \
    V(AddColumnFamily, add_column_family) \
    V(DropColumnFamily, drop_column_family)
    
class VersionPatch final {
public:
    #define DEFINE_ENUM(field, name) k##field,
    enum Field : int8_t {
        VERSION_FIELDS(DEFINE_ENUM)
        kMaxFields,
    };
    #undef DEFINE_ENUM
    
    VersionPatch() { Reset(); }
    ~VersionPatch() {}
    
    void set_last_sequence_number(core::SequenceNumber version) {
        set_field(kLastSequenceNumber);
        last_sequence_number_ = version;
    }
    
    void set_next_file_number(uint64_t number) {
        set_field(kNextFileNumber);
        next_file_number_ = number;
    }
    
    void set_redo_log(uint32_t cfid, uint64_t number) {
        set_field(kRedoLog);
        redo_log_.cfid = cfid;
        redo_log_.number = number;
    }
    
    void set_prev_log_number(uint64_t number) {
        set_field(kPrevLogNumber);
        prev_log_number_ = number;
    }
    
    void set_max_column_faimly(uint32_t max_column_family) {
        set_field(kMaxColumnFamily);
        max_column_family_ = max_column_family;
    }
    
    void set_compaction_point(uint32_t cfid, int level, std::string_view key) {
        set_field(kCompactionPoint);
        compaction_point_.cfid = cfid;
        compaction_point_.level = level;
        compaction_point_.key   = key;
    }
    
    void DeleteFile(uint32_t cfid, int level, uint64_t number) {
        set_field(kDeletion);
        file_deletion_.push_back({cfid, level, number});
    }

    void CreateFile(uint32_t cfid, int level, uint64_t file_number,
                    std::string smallest_key, std::string largest_key,
                    uint64_t file_size, uint64_t ctime) {
        FileMetaData *fmd = new FileMetaData{file_number};
        fmd->smallest_key = smallest_key;
        fmd->largest_key  = largest_key;
        fmd->size         = file_size;
        fmd->ctime        = ctime;
        CreaetFile(cfid, level, fmd);
    }
    
    void CreaetFile(uint32_t cfid, int level, FileMetaData *fmd) {
        set_field(kCreation);
        file_creation_.push_back({cfid, level, base::MakeRef(fmd)});
    }
    
    void DropColumnFamily(const uint32_t cfid) {
        set_field(kDropColumnFamily);
        cf_deletion_ = cfid;
    }
    
    void AddColumnFamily(const std::string name, uint32_t cfid,
                         const std::string comparator_name) {
        set_field(kAddColumnFamily);
        cf_creation_.cfid = cfid;
        cf_creation_.name = name;
        cf_creation_.comparator_name    = comparator_name;
        
    }
    
    #define DEFINE_TEST_FIELD(field, name) \
        bool has_##name() const { return has_field(k##field); }
    VERSION_FIELDS(DEFINE_TEST_FIELD)
    #undef DEFINE_TEST_FIELD
    
    bool has_field(Field field) const {
        int i = static_cast<int>(field);
        DCHECK_GE(i, 0); DCHECK_LT(i, kMaxFields);
        return fields_[i / 32] & (1 << (i % 32));
    }
    
    struct RedoLog {
        uint32_t  cfid;
        uint64_t  number;
    };
    
    struct CompactionPoint {
        uint32_t    cfid;
        int         level;
        std::string key;
    };
    
    struct CFCreation {
        uint32_t    cfid;
        std::string name;
        std::string comparator_name;
    };
    
    struct FileDeletion {
        uint32_t cfid;
        int      level;
        uint64_t number;
    };
    
    struct FileCreation {
        uint32_t cfid;
        int      level;
        base::Handle<FileMetaData> file_metadata;
    };
    
    typedef std::vector<FileCreation> FileCreationCollection;
    typedef std::vector<FileDeletion> FileDeletionCollection;
    
    DEF_VAL_GETTER(uint32_t, max_column_family);
    DEF_VAL_GETTER(core::SequenceNumber, last_sequence_number);
    DEF_VAL_GETTER(uint64_t, next_file_number);
    DEF_VAL_GETTER(RedoLog, redo_log);
    DEF_VAL_GETTER(uint64_t, prev_log_number);
    DEF_VAL_GETTER(CompactionPoint, compaction_point);
    DEF_VAL_GETTER(CFCreation, cf_creation);
    DEF_VAL_GETTER(uint32_t, cf_deletion);
    DEF_VAL_GETTER(FileCreationCollection, file_creation);
    DEF_VAL_GETTER(FileDeletionCollection, file_deletion);
    
    void Reset() {
        ::memset(fields_, 0, arraysize(fields_) * sizeof(uint32_t));
        file_creation_.clear();
        file_deletion_.clear();
    }
    
    void Encode(std::string *buf) const;
    void Decode(std::string_view buf);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(VersionPatch);
private:
    void set_field(Field field) {
        int i = static_cast<int>(field);
        DCHECK_GE(i, 0); DCHECK_LT(i, kMaxFields);
        fields_[i / 32] |= (1 << (i % 32));
    }
    
    uint32_t max_column_family_;
    
    uint32_t cf_deletion_;
    CFCreation cf_creation_;
    
    core::SequenceNumber last_sequence_number_;
    uint64_t next_file_number_;
    RedoLog redo_log_;
    uint64_t prev_log_number_;
    
    CompactionPoint compaction_point_;
    
    FileCreationCollection file_creation_;
    FileDeletionCollection file_deletion_;
    
    uint32_t fields_[(kMaxFields + 31) / 32];
}; // class VersionPatch
    
    
class Version final {
public:
    explicit Version(ColumnFamilyImpl *owner) : owns_(owner) {}
    
    size_t NumberLevelFiles(int level) {
        DCHECK_GE(level, 0);
        DCHECK_LT(level, Config::kMaxLevel);
        return files_[level].size();
    }
    
    size_t SizeLevelFiles(int level) {
        DCHECK_GE(level, 0);
        DCHECK_LT(level, Config::kMaxLevel);
        size_t size = 0;
        for (auto fmd : files_[level]) {
            size += fmd->size;
        }
        return size;
    }
    
    void GetOverlappingInputs(int level, std::string_view begin,
                              std::string_view end,
                              std::vector<base::Handle<FileMetaData>> *inputs);
    
    DEF_PTR_GETTER_NOTNULL(ColumnFamilyImpl, owns);
    DEF_PTR_GETTER(Version, next);
    DEF_PTR_GETTER(Version, prev);
    DEF_VAL_GETTER(int, compaction_level);
    DEF_VAL_GETTER(double, compaction_score);
    
    const std::vector<base::Handle<FileMetaData>> &level_files(int level) {
        DCHECK_GE(level, 0);
        DCHECK_LT(level, Config::kMaxLevel);
        return files_[level];
    }
    
    Error Get(const ReadOptions &opts, std::string_view key,
              core::SequenceNumber version, core::Tag *tag, std::string *value);
    
    friend class ColumnFamilyImpl;
    friend class VersionSet;
    friend class VersionBuilder;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Version);
private:
    ColumnFamilyImpl *const owns_;
    Version *next_ = nullptr;
    Version *prev_ = nullptr;
    int      compaction_level_ = -1;
    double   compaction_score_ = -1;
    std::vector<base::Handle<FileMetaData>> files_[Config::kMaxLevel];
}; // class Version
    

class VersionSet final {
public:
    VersionSet(const std::string &abs_db_path, const Options &options,
               TableCache *table_cache);
    ~VersionSet();
    
    DEF_VAL_GETTER(std::string, abs_db_path);
    DEF_PTR_GETTER_NOTNULL(Env, env);
    DEF_VAL_GETTER(core::SequenceNumber, last_sequence_number);
    DEF_VAL_GETTER(uint64_t, next_file_number);
    DEF_VAL_GETTER(uint64_t, prev_log_number);
    DEF_VAL_GETTER(uint64_t, manifest_file_number);
    
    core::SequenceNumber AddSequenceNumber(core::SequenceNumber add) {
        last_sequence_number_ += add;
        return last_sequence_number_;
    }
    
    uint64_t GenerateFileNumber() { return next_file_number_++; }
    
    Error Recovery(const std::map<std::string, ColumnFamilyOptions> &desc,
                   uint64_t file_number,
                   std::vector<uint64_t> *history);
    
    Error LogAndApply(const ColumnFamilyOptions &cf_opts,
                      VersionPatch *patch,
                      std::mutex *mutex);
    
    Error CreateManifestFile();
    Error WriteCurrentSnapshot();
    
    ColumnFamilySet *column_families() const { return column_families_.get(); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(VersionSet);
private:
    Error WritePatch(const VersionPatch &patch);
    void Finalize(Version *version);

    const std::string abs_db_path_;
    Env *const env_;
    std::unique_ptr<ColumnFamilySet> column_families_;
    const uint64_t block_size_;

    core::SequenceNumber last_sequence_number_ = 1;
    uint64_t next_file_number_ = 0;
    uint64_t prev_log_number_ = 0;
    uint64_t manifest_file_number_ = 0;
    
    std::unique_ptr<WritableFile> log_file_;
    std::unique_ptr<LogWriter> logger_;
}; // class VersionSet

} // namespace db
    
} // namespace mai


#endif // MAI_DB_VERSION_H_
