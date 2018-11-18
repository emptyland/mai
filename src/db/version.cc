#include "db/version.h"
#include "db/column-family.h"
#include "db/files.h"
#include "db/write-ahead-log.h"

namespace mai {
    
namespace db {
    
    

////////////////////////////////////////////////////////////////////////////////
/// class VersionPatch
////////////////////////////////////////////////////////////////////////////////

void VersionPatch::Encode(std::string *buf) const {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    ScopedMemory scope;
    if (has_field(kComparator)) {
        buf->append(Slice::GetByte(kComparator, &scope));
        buf->append(Slice::GetV32(column_family_id_, &scope));
        buf->append(Slice::GetString(comparator_name_, &scope));
    }
    if (has_field(kPrevLogNumber)) {
        buf->append(Slice::GetByte(kPrevLogNumber, &scope));
        buf->append(Slice::GetV64(prev_log_number_, &scope));
    }
    if (has_field(kRedoLogNumber)) {
        buf->append(Slice::GetByte(kRedoLogNumber, &scope));
        buf->append(Slice::GetV64(redo_log_number_, &scope));
    }
    if (has_field(kNextFileNumber)) {
        buf->append(Slice::GetByte(kNextFileNumber, &scope));
        buf->append(Slice::GetV64(next_file_number_, &scope));
    }
    if (has_field(kLastSequenceNumber)) {
        buf->append(Slice::GetByte(kLastSequenceNumber, &scope));
        buf->append(Slice::GetV64(last_sequence_number_, &scope));
    }
    if (has_field(kMaxColumnFamily)) {
        buf->append(Slice::GetByte(kMaxColumnFamily, &scope));
        buf->append(Slice::GetV32(max_column_family_, &scope));
    }
    if (has_field(kAddCloumnFamily)) {
        buf->append(Slice::GetByte(kAddCloumnFamily, &scope));
        buf->append(Slice::GetV32(column_family_id_, &scope));
        buf->append(Slice::GetString(column_family_name_, &scope));
        buf->append(Slice::GetString(comparator_name_, &scope));
    }
    if (has_field(kDropCloumnFamily)) {
        buf->append(Slice::GetByte(kDropCloumnFamily, &scope));
        buf->append(Slice::GetV64(column_family_id_, &scope));
    }
    if (has_field(kCompactionPoint)) {
        buf->append(Slice::GetByte(kCompactionPoint, &scope));
        buf->append(Slice::GetV32(column_family_id_, &scope));
        buf->append(Slice::GetV32(compaction_level_, &scope));
        buf->append(Slice::GetString(compaction_key_, &scope));
    }
    if (has_field(kCreation)) {
        for (const auto &c : creation_) {
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
    if (has_field(kDeletion)) {
        for (const auto &d : deletion_) {
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
            case kComparator: {
                uint32_t cfid = reader.ReadVarint32();
                std::string name(reader.ReadString());
                set_comparator(cfid, name);
            } break;
                
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
                
            case kAddCloumnFamily: {
                uint32_t cfid = reader.ReadVarint32();
                std::string name(reader.ReadString());
                std::string comparator(reader.ReadString());
                AddColumnFamily(name, cfid, comparator);
            } break;
                
            case kDropCloumnFamily: {
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
/// class VersionSet
////////////////////////////////////////////////////////////////////////////////
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
