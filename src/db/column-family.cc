#include "db/column-family.h"
#include "db/version.h"
#include "db/files.h"
#include "db/factory.h"
#include "db/table-cache.h"
#include "db/compaction.h"
#include "db/config.h"
#include "mai/iterator.h"

namespace mai {
    
namespace db {
    
// Stores the minimal range that covers all entries in inputs in
// *smallest, *largest.
// REQUIRES: inputs is not empty
void GetRange(const std::vector<base::intrusive_ptr<FileMetaData>> &inputs,
              const core::InternalKeyComparator *ikcmp,
              std::string *smallest, std::string *largest) {
    DCHECK(!inputs.empty());
    smallest->clear();
    largest->clear();
    for (size_t i = 0; i < inputs.size(); i++) {
        base::intrusive_ptr<FileMetaData> fmd = inputs[i];
        if (i == 0) {
            *smallest = fmd->smallest_key;
            *largest = fmd->largest_key;
        } else {
            if (ikcmp->Compare(fmd->smallest_key, *smallest) < 0) {
                *smallest = fmd->smallest_key;
            }
            if (ikcmp->Compare(fmd->largest_key, *largest) > 0) {
                *largest = fmd->largest_key;
            }
        }
    }
}

// Stores the minimal range that covers all entries in inputs1 and inputs2
// in *smallest, *largest.
// REQUIRES: inputs is not empty
    void GetRange2(const std::vector<base::intrusive_ptr<FileMetaData>> &inputs1,
                   const std::vector<base::intrusive_ptr<FileMetaData>> &inputs2,
                   const core::InternalKeyComparator *ikcmp,
                   std::string *smallest, std::string *largest) {
    std::vector<base::intrusive_ptr<FileMetaData>> all = inputs1;
    all.insert(all.end(), inputs2.begin(), inputs2.end());
    GetRange(all, ikcmp, smallest, largest);
}
    
////////////////////////////////////////////////////////////////////////////////
/// class ColumnFamilyImpl
////////////////////////////////////////////////////////////////////////////////
    
ColumnFamilyImpl::ColumnFamilyImpl(const std::string &name, uint32_t id,
                                   const ColumnFamilyOptions &options,
                                   VersionSet *versions,
                                   ColumnFamilySet *owner)
    : name_(name)
    , id_(id)
    , options_(options)
    , dummy_versions_(new Version(this))
    , current_(dummy_versions_)
    , ikcmp_(DCHECK_NOTNULL(options.comparator))
    , owns_(owner)
    , ref_count_(0)
    , dropped_(false)
    , background_progress_(false) {
    AddRef();
    dummy_versions_->next_ = dummy_versions_;
    dummy_versions_->prev_ = dummy_versions_;
}
    
/*virtual*/ ColumnFamilyImpl::~ColumnFamilyImpl() {
    DCHECK_EQ(ref_count_.load(std::memory_order_relaxed), 0);
    // remove from linked list
    ColumnFamilyImpl *prev = prev_;
    ColumnFamilyImpl *next = next_;
    prev->next_ = next;
    next->prev_ = prev;
    
    if (!dropped_ && owns_) {
        owns_->RemoveColumnFamily(this);
    }
    
    while (dummy_versions_->next() != dummy_versions_) {
        Version *x = dummy_versions_->next();
        Version *prev = x->prev();
        Version *next = x->next();
        prev->next_ = next;
        next->prev_ = prev;
        delete x;
    }
    // TODO:
}
    
void ColumnFamilyImpl::Drop() {
    DCHECK_NE(id_, 0) << "Can not drop default column family!";
    dropped_ = true;
    
    if (owns_) {
        owns_->RemoveColumnFamily(this);
    }
}
    
void ColumnFamilyImpl::MakeImmutablePipeline(Factory *factory,
                                             uint64_t redo_log_number) {
    mutable_->set_associated_file_number(redo_log_number);
    immutable_pipeline_.Add(mutable_);
    mutable_ = factory->NewMemoryTable(&ikcmp_,
                                       owns_->env()->GetLowLevelAllocator(),
                                       options_.use_unordered_table,
                                       options_.number_of_hash_slots);
}
    
void ColumnFamilyImpl::Append(Version *version) {
    // add linked list
    version->next_ = dummy_versions_;
    Version *prev = dummy_versions_->prev_;
    version->prev_ = prev;
    prev->next_ = version;
    dummy_versions_->prev_ = version;
    current_ = version;
}
    
bool ColumnFamilyImpl::NeedsCompaction() const {
//    if (current()->NumberLevelFiles(0) > Config::kMaxNumberLevel0File) {
//        return true;
//    }
//
//    if (current()->SizeLevelFiles(0) > Config::kMaxSizeLevel0File) {
//        return true;
//    }
//
//    for (auto i = 1; i < Config::kMaxLevel; i++) {
//        if (current()->SizeLevelFiles(i) > Config::kMaxSizeLevel0File * (i + 1)) {
//            return true;
//        }
//    }
//    return false;
    return current()->compaction_score_ >= 1.0;
}
    
bool ColumnFamilyImpl::PickCompaction(CompactionContext *ctx) {
    if (!NeedsCompaction()) {
        return false;
    }

    bool size_compaction = (current()->compaction_score_ >= 1.0);
    DCHECK_NOTNULL(ctx);
    if (size_compaction) {
        ctx->level = current_->compaction_level_;
        DCHECK_GE(ctx->level, 0);
        DCHECK_LT(ctx->level + 1, Config::kMaxLevel);

        for (auto fmd : current_->level_files(ctx->level)) {
            if (compaction_point_[ctx->level].empty() ||
                ikcmp_.Compare(compaction_point_[ctx->level],
                               fmd->largest_key) < 0) {
                ctx->inputs[0].push_back(fmd);
                break;
            }
        }
        
        if (ctx->inputs[0].empty()) {
            ctx->inputs[0].push_back(current_->files_[ctx->level][0]);
        }
    } else {
        return false;
    }
    
    ctx->input_version = current_;
    
    // Files in level 0 may overlap each other, so pick up all overlapping ones
    if (ctx->level == 0) {
        std::string smallest, largest;
        GetRange(ctx->inputs[0], &ikcmp_, &smallest, &largest);
        // Note that the next call will discard the file we placed in
        // c->inputs_[0] earlier and replace it with an overlapping set
        // which will include the picked file.
        current_->GetOverlappingInputs(0, smallest, largest, &ctx->inputs[0]);
        DCHECK(!ctx->inputs[0].empty());
    }
    
    SetupOtherInputs(ctx);
    return true;
}
    
void ColumnFamilyImpl::SetupOtherInputs(CompactionContext *ctx) {
    const int level = ctx->level;
    std::string smallest, largest;
    GetRange(ctx->inputs[0], ikcmp(), &smallest, &largest);
    
    current_->GetOverlappingInputs(level + 1, smallest, largest,
                                   &ctx->inputs[1]);
    
    // Get entire range covered by compaction
    std::string all_start, all_limit;
    GetRange2(ctx->inputs[0], ctx->inputs[1], ikcmp(), &all_start, &all_limit);
    
    // TODO:
    
    // Update the place where we will do the next compaction for this level.
    // We update this immediately instead of waiting for the VersionEdit
    // to be applied so that if the compaction fails, we will try a different
    // key range next time.
    //compaction_pointer_[level] = largest;
    compaction_point_[level] = largest;
    ctx->patch.set_compaction_point(id(), level, largest);
}

Error ColumnFamilyImpl::Install(Factory *factory) {
    // TODO:
    mutable_ = factory->NewMemoryTable(&ikcmp_,
                                       owns_->env()->GetLowLevelAllocator(),
                                       options_.use_unordered_table,
                                       options_.number_of_hash_slots);
    std::string cfdir = GetDir();
    Error rs = owns_->env()->MakeDirectory(cfdir, false);
    if (!rs) {
        return rs;
    }
    
    initialized_ = true;
    return Error::OK();
}
    
Error ColumnFamilyImpl::Uninstall() {
    DCHECK(initialized());
    DCHECK(!background_progress());
    DCHECK(dropped());
    
    std::string cfdir = GetDir();
    Error rs = owns_->env()->DeleteFile(cfdir, true);
    if (!rs) {
        return rs;
    }
    initialized_ = false;
    return Error::OK();
}

std::string ColumnFamilyImpl::GetDir() const {
    if (options().dir.empty()) {
        return owns_->abs_db_path() + "/" + name_;
    } else {
        return owns_->env()->GetAbsolutePath(options().dir) + "/" + name_;
    }
}
    
std::string ColumnFamilyImpl::GetTableFileName(uint64_t file_number) const {
    return Files::TableFileName(GetDir(), options_.use_unordered_table ?
                                Files::kS1T_Table : Files::kSST_Table,
                                file_number);
}
    
Error ColumnFamilyImpl::AddIterators(const ReadOptions &opts,
                                    std::vector<Iterator *> *result) {
    Error rs;
    for (int i = 0; i < Config::kMaxLevel; ++i) {
        for (const auto &fmd : current()->level_files(i)) {
            std::unique_ptr<Iterator>
                iter(owns_->table_cache()->NewIterator(opts, this, fmd->number,
                                                       fmd->size));
            if (iter->error().fail()) {
                return iter->error();
            }
            result->push_back(iter.release());
        }
    }
    return rs;
}

////////////////////////////////////////////////////////////////////////////////
/// class ColumnFamilyHandle
////////////////////////////////////////////////////////////////////////////////
    
/*virtual*/ ColumnFamilyHandle::~ColumnFamilyHandle() {}

/*virtual*/ std::string ColumnFamilyHandle::name() const {
    return impl_->name();
}

/*virtual*/ uint32_t ColumnFamilyHandle::id() const {
    return impl_->id();
}

/*virtual*/ const Comparator *ColumnFamilyHandle::comparator() const {
    return impl_->ikcmp()->ucmp();
}
    
/*virtual*/ Error
ColumnFamilyHandle::GetDescriptor(ColumnFamilyDescriptor *desc) const {
    if (impl_->dropped()) {
        return MAI_CORRUPTION("Column family has dropped!");
    }
    desc->name    = impl_->name();
    desc->options = impl_->options();
    return Error::OK();
}

////////////////////////////////////////////////////////////////////////////////
/// class ColumnFamilySet
////////////////////////////////////////////////////////////////////////////////

ColumnFamilySet::ColumnFamilySet(VersionSet *owns,
                                 TableCache *table_cache)
    : owns_(DCHECK_NOTNULL(owns))
    , table_cache_(DCHECK_NOTNULL(table_cache))
    , dummy_cfd_(new ColumnFamilyImpl("", 0, ColumnFamilyOptions{}, nullptr,
                                      nullptr)) {
    dummy_cfd_->prev_ = dummy_cfd_;
    dummy_cfd_->next_ = dummy_cfd_;
}

ColumnFamilySet::~ColumnFamilySet() {
    while (column_family_impls_.size() > 0) {
        // cfd destructor will delete itself from column_family_data_
        ColumnFamilyImpl *cfd = column_family_impls_.begin()->second;
        bool last_ref = cfd->ReleaseRef();
        DCHECK(last_ref); (void)last_ref;
        delete cfd;
    }
    bool dummy_last_ref = dummy_cfd_->ReleaseRef();
    DCHECK(dummy_last_ref); (void)dummy_last_ref;
    delete dummy_cfd_;
}
    
const std::string &ColumnFamilySet::abs_db_path() const {
    return owns_->abs_db_path();
}
    
Env *ColumnFamilySet::env() const {
    return owns_->env();
}

ColumnFamilyImpl *ColumnFamilySet::NewColumnFamily(const ColumnFamilyOptions opts,
                                                   const std::string &name,
                                                   uint32_t id,
                                                   VersionSet *versions) {
    DCHECK(column_families_.find(name) == column_families_.end());
    DCHECK(column_family_impls_.find(id) == column_family_impls_.end());
    
    ColumnFamilyImpl *cfd = new ColumnFamilyImpl(name, id, opts, versions, this);
    
    column_families_.insert({name, id});
    column_family_impls_.insert({id, cfd});
    max_column_family_ = std::max(id, max_column_family_);
    
    // add linked list
    cfd->next_ = dummy_cfd_;
    ColumnFamilyImpl *prev = dummy_cfd_->prev_;
    cfd->prev_ = prev;
    prev->next_ = cfd;
    dummy_cfd_->prev_ = cfd;

    if (id == 0) {
        default_cfd_ = cfd;
    }
    return cfd;
}

} // namespace db
    
} // namespace mai
