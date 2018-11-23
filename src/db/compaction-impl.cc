#include "db/compaction-impl.h"
#include "db/table-cache.h"
#include "db/column-family.h"
#include "db/version.h"
#include "db/config.h"
#include "table/table-builder.h"
#include "core/merging.h"
#include "core/key-boundle.h"
#include "core/key-filter.h"
#include "mai/iterator.h"

namespace mai {

namespace db {
    
CompactionImpl::CompactionImpl(const std::string abs_db_path,
                               const core::InternalKeyComparator *ikcmp,
                               TableCache *table_cache, ColumnFamilyImpl *cfd)
    : Compaction(cfd)
    , abs_db_path_(abs_db_path)
    , ikcmp_(DCHECK_NOTNULL(ikcmp))
    , table_cache_(DCHECK_NOTNULL(table_cache)) {}
    
/*virtual*/ CompactionImpl::~CompactionImpl() {
    
}

/*virtual*/ Error CompactionImpl::Run(table::TableBuilder *builder) {
    std::unique_ptr<Iterator>
        merger(core::Merging::NewMergingIterator(ikcmp_, mutable_original_input()->data(),
                                                 original_input().size()));
    if (merger->error().fail()) {
        return merger->error();
    }
    if (compaction_point().empty()) {
        merger->SeekToFirst();
    } else {
        merger->Seek(compaction_point());
    }
    
    bool last_level = (target_level() == (Config::kMaxLevel - 1));
    std::string deletion_key;
    core::SequenceNumber deletion_sequence_number = 0;
    core::ParsedTaggedKey ikey;
    for (;merger->Valid(); merger->Next()) {
        core::KeyBoundle::ParseTaggedKey(merger->key(), &ikey);
        
        if (ikey.tag.sequence_number() < oldest_sequence_number()) {
            continue; // Too old sequence number shoud be drop.
        }
        
        if (ikey.tag.flag() == core::Tag::kFlagDeletion) {
            deletion_key = ikey.user_key;
            deletion_sequence_number = ikey.tag.sequence_number();
        }
        
        bool should_delete = false;
        if (ikcmp_->ucmp()->Equals(ikey.user_key, deletion_key) &&
            ikey.tag.sequence_number() <= deletion_sequence_number) {
            should_delete = true;
        }
        
        if (last_level) {
            if (!should_delete) {
                std::string key;
                core::KeyBoundle::MakeKey(ikey.user_key, 0);
                builder->Add(merger->key(), merger->value());
            }
        } else {
            if (should_delete) {
                should_delete = EnsureKeyNotExists(target_level(), merger->key());
            }
            
            // TODO:
        }
    }
    return Error::OK();
}
    
bool CompactionImpl::EnsureKeyNotExists(int start_level, std::string_view key) {
    for (int i = start_level; i < Config::kMaxLevel; ++i) {
        for (auto fmd : cfd()->current()->level_files(i)) {
            if (ikcmp_->Compare(key, fmd->largest_key) > 0 ||
                ikcmp_->Compare(key, fmd->smallest_key) < 0) {
                continue;
            }
            
            std::shared_ptr<core::KeyFilter> filter;
            Error rs = table_cache_->GetKeyFilter(cfd(), fmd->number, &filter);
            if (!rs) {
                return false;
            }
            if (filter->MayExists(key)) {
                return false;
            }
        }
    }
    return true;
}
    
} // namespace db

} // namespace mai


