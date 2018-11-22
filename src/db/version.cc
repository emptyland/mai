#include "db/version.h"
#include "db/column-family.h"
#include "db/files.h"
#include "db/write-ahead-log.h"
#include "db/table-cache.h"
#include "core/merging.h"
#include "base/io-utils.h"
#include "mai/iterator.h"

namespace mai {
    
namespace db {

////////////////////////////////////////////////////////////////////////////////
/// class VersionBuilder
////////////////////////////////////////////////////////////////////////////////
    
class VersionBuilder final {
public:
    VersionBuilder(VersionSet *versions)
        : owns_(DCHECK_NOTNULL(versions)) {
    }
    
    ColumnFamilyImpl *column_family() const { return cf_.get(); }
    
    void Apply(const VersionPatch &patch) {
        for (const auto &d : patch.file_deletion()) {
            levels_[d.level].deletion.insert(d.number);
        }
        
        for (const auto &c : patch.file_creation()) {
            levels_[c.level].deletion.erase(c.file_metadata->number);
            levels_[c.level].creation.emplace(c.file_metadata.get());
        }
    }
    
    Version *Build() {
        std::unique_ptr<Version> version(new Version(cf_.get()));
        
        for (auto i = 0; i < kMaxLevel; i++) {
            
            auto level = cf_->current()->level_files(i);
            
            for (auto fmd : level) {
                
                if (levels_[i].deletion.find(fmd->number) ==
                    levels_[i].deletion.end()) {
                    version->files_[i].push_back(fmd);
                }
            }
            levels_[i].deletion.clear();
            
            for (auto fmd : levels_[i].creation) {
                version->files_[i].push_back(fmd);
            }
            levels_[i].creation.clear();
        }
        return version.release();
    }
    
    void Prepare(const VersionPatch &patch) {
        for (const auto &c : patch.file_creation()) {
            if (!cf_) {
                cf_ = owns_->column_families()->GetColumnFamily(c.cfid);
            } else {
                DCHECK_EQ(cf_->id(), c.cfid);
            }
        }
        for (const auto &d : patch.file_deletion()) {
            if (!cf_) {
                cf_ = owns_->column_families()->GetColumnFamily(d.cfid);
            } else {
                DCHECK_EQ(cf_->id(), d.cfid);
            }
        }
        BySmallestKey cmp{cf_->ikcmp()};
        for (auto i = 0; i < kMaxLevel; i++) {
            levels_[i].creation = std::set<base::Handle<FileMetadata>,
            BySmallestKey>(cmp);
        }
    }
    
private:
    struct BySmallestKey {
        
        const core::InternalKeyComparator *ikcmp;
        
        bool operator ()(const base::Handle<FileMetadata> &a,
                         const base::Handle<FileMetadata> &b) const {
            int rv = ikcmp->Compare(a->smallest_key, b->smallest_key);
            if (rv != 0) {
                return rv < 0;
            } else {
                return a->number < b->number;
            }
        }
    };
    
    struct FileEntry {
        std::set<uint64_t> deletion;
        std::set<base::Handle<FileMetadata>, BySmallestKey> creation;
    };
    
    VersionSet *owns_;
    FileEntry levels_[kMaxLevel];
    base::Handle<ColumnFamilyImpl> cf_;
}; // class VersionBuilder
    

////////////////////////////////////////////////////////////////////////////////
/// class VersionPatch
////////////////////////////////////////////////////////////////////////////////

void VersionPatch::Encode(std::string *buf) const {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    ScopedMemory scope;
    if (has_prev_log_number()) {
        buf->append(Slice::GetByte(kPrevLogNumber, &scope));
        buf->append(Slice::GetV64(prev_log_number_, &scope));
    }
    if (has_redo_log_number()) {
        buf->append(Slice::GetByte(kRedoLogNumber, &scope));
        buf->append(Slice::GetV64(redo_log_number_, &scope));
    }
    if (has_next_file_number()) {
        buf->append(Slice::GetByte(kNextFileNumber, &scope));
        buf->append(Slice::GetV64(next_file_number_, &scope));
    }
    if (has_last_sequence_number()) {
        buf->append(Slice::GetByte(kLastSequenceNumber, &scope));
        buf->append(Slice::GetV64(last_sequence_number_, &scope));
    }
    if (has_max_column_family()) {
        buf->append(Slice::GetByte(kMaxColumnFamily, &scope));
        buf->append(Slice::GetV32(max_column_family_, &scope));
    }
    if (has_add_column_family()) {
        buf->append(Slice::GetByte(kAddColumnFamily, &scope));
        buf->append(Slice::GetV32(cf_creation_.cfid, &scope));
        buf->append(Slice::GetString(cf_creation_.name, &scope));
        buf->append(Slice::GetString(cf_creation_.comparator_name, &scope));
    }
    if (has_drop_column_family()) {
        buf->append(Slice::GetByte(kDropColumnFamily, &scope));
        buf->append(Slice::GetV64(cf_deletion_, &scope));
    }
    if (has_compaction_point()) {
        buf->append(Slice::GetByte(kCompactionPoint, &scope));
        buf->append(Slice::GetV32(compaction_point_.cfid, &scope));
        buf->append(Slice::GetV32(compaction_point_.level, &scope));
        buf->append(Slice::GetString(compaction_point_.key, &scope));
    }
    if (has_creation()) {
        for (const auto &c : file_creation_) {
            buf->append(Slice::GetByte(kCreation, &scope));
            buf->append(Slice::GetV32(c.cfid, &scope));
            buf->append(Slice::GetV32(c.level, &scope));
            buf->append(Slice::GetV64(c.file_metadata->number, &scope));
            buf->append(Slice::GetString(c.file_metadata->largest_key, &scope));
            buf->append(Slice::GetString(c.file_metadata->smallest_key, &scope));
            buf->append(Slice::GetV64(c.file_metadata->size, &scope));
            buf->append(Slice::GetV64(c.file_metadata->ctime, &scope));
        }
    }
    if (has_deletion()) {
        for (const auto &d : file_deletion_) {
            buf->append(Slice::GetByte(kDeletion, &scope));
            buf->append(Slice::GetV32(d.cfid, &scope));
            buf->append(Slice::GetV32(d.level, &scope));
            buf->append(Slice::GetV64(d.number, &scope));
        }
    }
}

void VersionPatch::Decode(std::string_view buf) {
    base::BufferReader reader(buf);
    
    while (!reader.Eof()) {
        switch (static_cast<Field>(reader.ReadByte())) {
            case kPrevLogNumber: {
                uint64_t number = reader.ReadVarint64();
                set_prev_log_number(number);
            } break;
                
            case kRedoLogNumber: {
                uint64_t number = reader.ReadVarint64();
                set_redo_log_number(number);
            } break;
                
            case kNextFileNumber: {
                uint64_t number = reader.ReadVarint64();
                set_next_file_number(number);
            } break;
                
            case kMaxColumnFamily: {
                uint32_t max_cf = reader.ReadVarint32();
                set_max_column_faimly(max_cf);
            } break;
                
            case kLastSequenceNumber: {
                uint64_t number = reader.ReadVarint64();
                set_last_sequence_number(number);
            } break;
                
            case kCompactionPoint: {
                uint32_t cfid = reader.ReadVarint32();
                int level = reader.ReadVarint32();
                std::string key(reader.ReadString());
                set_compaction_point(cfid, level, key);
            } break;
                
            case kAddColumnFamily: {
                uint32_t cfid = reader.ReadVarint32();
                std::string name(reader.ReadString());
                std::string comparator(reader.ReadString());
                AddColumnFamily(name, cfid, comparator);
            } break;
                
            case kDropColumnFamily: {
                uint32_t cfid = reader.ReadVarint32();
                DropColumnFamily(cfid);
            } break;
                
            case kDeletion: {
                uint32_t cfid = reader.ReadVarint32();
                int level = reader.ReadVarint32();
                uint64_t file_number = reader.ReadVarint64();
                DeleteFile(cfid, level, file_number);
            } break;
                
            case kCreation: {
                uint32_t cfid = reader.ReadVarint32();
                int level = reader.ReadVarint32();
                uint64_t file_number = reader.ReadVarint64();
                
                FileMetadata *fmd = new FileMetadata(file_number);
                fmd->largest_key.assign(reader.ReadString());
                fmd->smallest_key.assign(reader.ReadString());
                fmd->size = reader.ReadVarint64();
                fmd->ctime = reader.ReadVarint64();
                CreaetFile(cfid, level, fmd);
            } break;
                
            default:
                DLOG(FATAL) << "Noreaced!";
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/// class Version
////////////////////////////////////////////////////////////////////////////////
Error Version::Get(const ReadOptions &opts, std::string_view key,
                   core::SequenceNumber version, core::Tag *tag,
                   std::string *value) {
    const core::InternalKeyComparator *const ikcmp = owns_->ikcmp();
    
    base::ScopedMemory scope;
    const core::KeyBoundle *const ikey
        = core::KeyBoundle::New(key, version, base::ScopedAllocator{&scope});
    
    std::vector<base::Handle<FileMetadata>> maybe_files;
    for (const auto &metadata : level_files(0)) {
        if (ikcmp->Compare(ikey->key(), metadata->smallest_key) >= 0 ||
            ikcmp->Compare(ikey->key(), metadata->largest_key) <= 0) {
            maybe_files.push_back(metadata);
        }
    }
    // The newest file should be first.
    std::sort(maybe_files.begin(), maybe_files.end(),
              [](const base::Handle<FileMetadata> &a,
                 const base::Handle<FileMetadata> &b) {
                  return a->ctime > b->ctime;
              });
    
    for (int i = 1; i < kMaxLevel; ++i) {
        if (level_files(i).empty()) {
            continue;
        }
        
        for (const auto &metadata : level_files(i)) {
            if (ikcmp->Compare(ikey->key(), metadata->smallest_key) >= 0 ||
                ikcmp->Compare(ikey->key(), metadata->largest_key) <= 0) {
                maybe_files.push_back(metadata);
            }
        }
    }
    if (maybe_files.empty()) {
        return MAI_NOT_FOUND("No files");
    }
    
    std::vector<Iterator*> iters;
    for (const auto &fmd : maybe_files) {
        Iterator *iter =
            owns_->owns()->table_cache()->NewIterator(opts, owns_, fmd->number,
                                                      fmd->size);
        if (iter->error().fail()) {
            delete iter;
            for (auto i : iters) {
                delete i;
            }
            return iter->error();
        }
        iters.push_back(iter);
    }
    
    std::unique_ptr<Iterator>
        merger(core::Merging::NewMergingIterator(ikcmp, iters.data(),
                                                 iters.size()));
    if (merger->error().fail()) {
        return merger->error();
    }
    merger->Seek(ikey->key());
    if (!merger->Valid()) {
        return MAI_NOT_FOUND("Seek()");
    }
    
    core::ParsedTaggedKey tkey;
    core::KeyBoundle::ParseTaggedKey(merger->key(), &tkey);
    if (!ikcmp->ucmp()->Equals(tkey.user_key, ikey->user_key())) {
        return MAI_NOT_FOUND("Target not found");
    }
    if (tkey.tag.flags() == core::Tag::kFlagDeletion) {
        return MAI_NOT_FOUND("Deleted");
    }
    
    value->assign(merger->value());
    if (tag) { *tag = tkey.tag; }
    return Error::OK();
}

////////////////////////////////////////////////////////////////////////////////
/// class VersionSet
////////////////////////////////////////////////////////////////////////////////
VersionSet::VersionSet(const std::string db_name, const Options &options,
                       TableCache *table_cache)
    : db_name_(db_name)
    , env_(DCHECK_NOTNULL(options.env))
    , column_families_(new ColumnFamilySet(db_name, table_cache))
    , block_size_(options.block_size) {
}

VersionSet::~VersionSet() {    
}
    
Error VersionSet::Recovery(const std::map<std::string, ColumnFamilyOptions> &desc,
                           uint64_t file_number,
                           std::vector<uint64_t> *logs) {
    
    std::string file_name = Files::ManifestFileName(db_name_, file_number);
    std::unique_ptr<SequentialFile> file;
    
    Error rs = env_->NewSequentialFile(file_name, &file);
    if (!rs) {
        return rs;
    }
    
    VersionPatch patch;
    std::string_view record;
    std::string scratch;
    LogReader reader(file.get(), true, WAL::kDefaultBlockSize);
    while (reader.Read(&record, &scratch)) {
        patch.Reset();
        patch.Decode(record);
        
        if (patch.has_add_column_family()) {
            auto iter = desc.find(patch.cf_creation().name);
            if (iter == desc.end()) {
                return MAI_CORRUPTION(std::string("Column family options not found: ")
                                      + patch.cf_creation().name);
            }
            if (patch.cf_creation().comparator_name
                .compare(iter->second.comparator->Name()) != 0) {
                return MAI_CORRUPTION("Incorrect comparator name!");
            }
            column_families_->NewColumnFamily(iter->second,
                                              patch.cf_creation().name,
                                              patch.cf_creation().cfid, this);
        }
        if (patch.has_drop_column_family()) {
            uint32_t id = patch.cf_deletion();
            DCHECK_NE(0, id);
            ColumnFamilyImpl *cfd = column_families_->GetColumnFamily(id);
            DCHECK_NOTNULL(cfd);
            cfd->Drop();
        }
        if (patch.has_deletion() || patch.has_creation()) {
            VersionBuilder builder(this);
            builder.Prepare(patch);
            builder.Apply(patch);
            builder.column_family()->Append(builder.Build());
        }
        if (patch.has_last_sequence_number()) {
            logs->push_back(patch.last_sequence_number());
        }
        if (patch.has_redo_log_number()) {
            redo_log_number_ = patch.redo_log_number();
        }
        if (patch.has_prev_log_number()) {
            prev_log_number_ = patch.prev_log_number();
        }
        if (patch.has_next_file_number()) {
            next_file_number_ = patch.next_file_number();
        }
        if (patch.has_last_sequence_number()) {
            last_sequence_number_ = patch.last_sequence_number();
        }
        if (patch.has_max_column_family()) {
            column_families_->UpdateColumnFamilyId(patch.max_column_family());
        }
    }
    if (!reader.error().ok() && !reader.error().IsEof()) {
        return reader.error();
    }
    
    return Error::OK();
}
    
Error VersionSet::LogAndApply(const ColumnFamilyOptions &cf_opts,
                              VersionPatch *patch,
                              std::mutex *mutex) {
    Error rs;
    
    if (patch->has_redo_log_number()) {
        DCHECK_GE(patch->redo_log_number(), redo_log_number_);
        DCHECK_LT(patch->redo_log_number(), next_file_number_);
    } else {
        patch->set_redo_log_number(redo_log_number_);
    }
    
    if (!patch->has_prev_log_number()) {
        patch->set_prev_log_number(prev_log_number_);
    }
    
    patch->set_last_sequence_number(last_sequence_number_);
    patch->set_next_file_number(next_file_number_);
    
    if (patch->has_add_column_family()) {
        column_families_->NewColumnFamily(cf_opts,
                                          patch->cf_creation().name,
                                          patch->cf_creation().cfid, this);
        patch->set_max_column_faimly(column_families_->max_column_family());
    }
    
    if (patch->has_drop_column_family()) {
        uint32_t id = patch->cf_deletion();
        DCHECK_NE(0, id);
        ColumnFamilyImpl *cfd = column_families_->GetColumnFamily(id);
        DCHECK_NOTNULL(cfd);
        cfd->Drop();
    }
    
    if (log_file_.get() == nullptr) {
        rs = CreateManifestFile();
        if (!rs) {
            return rs;
        }
        patch->set_next_file_number(next_file_number_);
    }
    rs = WritePatch(*patch);
    if (!rs) {
        return rs;
    }
    
    if (patch->has_deletion() || patch->has_creation()) {
        VersionBuilder builder(this);
        builder.Prepare(*patch);
        builder.Apply(*patch);
        builder.column_family()->Append(builder.Build());
    }
    
    redo_log_number_ = patch->redo_log_number();
    prev_log_number_ = patch->prev_log_number();
    return Error::OK();
}
    
Error VersionSet::CreateManifestFile() {
    DCHECK(!log_file_);
    
    manifest_file_number_ = GenerateFileNumber();
    std::string file_name = Files::ManifestFileName(db_name_,
                                                    manifest_file_number_);
    Error rs = env_->NewWritableFile(file_name, true, &log_file_);
    if (!rs) {
        return rs;
    }
    //log_file_->Truncate(0);
    logger_.reset(new LogWriter(log_file_.get(), block_size_));
    
    return WriteCurrentSnapshot();
}
    
Error VersionSet::WriteCurrentSnapshot() {
    VersionPatch patch;
    
    for (ColumnFamilyImpl *cf : *column_families_) {
        patch.Reset();
        patch.AddColumnFamily(cf->name(), cf->id(), cf->ikcmp()->ucmp()->Name());
        
        for (int i = 0; i < kMaxLevel; ++i) {
            for (auto fmd : cf->current()->level_files(i)) {
                patch.CreaetFile(cf->id(), i, fmd.get());
            }
        }
        Error rs = WritePatch(patch);
        if (!rs) {
            return rs;
        }
    }
    
    patch.Reset();
    patch.set_max_column_faimly(column_families_->max_column_family());
    patch.set_last_sequence_number(last_sequence_number_);
    patch.set_next_file_number(next_file_number_);
    patch.set_prev_log_number(prev_log_number_);
    patch.set_redo_log_number(redo_log_number_);
    
    char manifest[64];
    ::snprintf(manifest, arraysize(manifest), "%llu", manifest_file_number_);
    Error rs = base::FileWriter::WriteAll(Files::CurrentFileName(db_name_),
                                          manifest, env_);
    if (!rs) {
        return rs;
    }
    return WritePatch(patch);
}
    
Error VersionSet::WritePatch(const VersionPatch &patch) {
    std::string buf;
    
    patch.Encode(&buf);
    
    Error rs = logger_->Append(buf);
    if (!rs) {
        return rs;
    }
    return logger_->Sync(true);
}
    
} // namespace db
    
} // namespace mai
