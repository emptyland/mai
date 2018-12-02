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
    for (auto iter : original_input()) {
        delete iter;
    }
}

/*virtual*/ Error CompactionImpl::Run(table::TableBuilder *builder,
                                      CompactionResult *result) {
    std::unique_ptr<Iterator>
        merger(core::Merging::NewMergingIterator(ikcmp_,
                                                 mutable_original_input()->data(),
                                                 original_input().size()));
    if (merger->error().fail()) {
        return merger->error();
    }
    mutable_original_input()->clear();

//    if (compaction_point().empty()) {
        merger->SeekToFirst();
//    } else {
//        merger->Seek(compaction_point());
//    }
    
    bool last_level = (target_level() == (Config::kMaxLevel - 1));
    std::string deletion_key;
    core::SequenceNumber deletion_sequence_number = 0;
    core::ParsedTaggedKey ikey;
    for (;merger->Valid(); merger->Next()) {
        core::KeyBoundle::ParseTaggedKey(merger->key(), &ikey);
        
        if (ikey.tag.sequence_number() >= oldest_sequence_number()) {
            continue; // Too old sequence number shoud be drop.
        }
        
        if (ikey.tag.flag() == core::Tag::kFlagDeletion) {
            deletion_key = ikey.user_key;
            deletion_sequence_number = ikey.tag.sequence_number();
        }
        
        bool drop = false;
        if (ikcmp_->ucmp()->Equals(ikey.user_key, deletion_key) &&
            ikey.tag.sequence_number() <= deletion_sequence_number) {
            drop = true;
        }
        
        if (last_level) {
            if (drop) {
                result->deletion_keys++;
                result->deletion_size += merger->key().size();
                result->deletion_size += merger->value().size();
            } else {
                std::string key = core::KeyBoundle::MakeKey(ikey.user_key, 0,
                                                            core::Tag::kFlagValue);
                builder->Add(key, merger->value());
                result->compacted_size += (key.size() + merger->value().size());
            }
        } else {
//            if (drop) {
//                Error rs = EnsureKeyNotExists(target_level(), merger->key(),
//                                              &drop);
//                if (!rs) {
//                    return rs;
//                }
//            }
            
            // TODO:
            
            if (drop) {
                result->deletion_keys++;
                result->deletion_size += merger->key().size();
                result->deletion_size += merger->value().size();
            } else {
                builder->Add(merger->key(), merger->value());
                result->compacted_size += (merger->key().size() +
                                           merger->value().size());
            }
        }
        
        if (!drop) {
            if (result->smallest_key.empty() ||
                ikcmp_->Compare(merger->key(), result->smallest_key) < 0) {
                result->smallest_key = merger->key();
            }
            if (result->largest_key.empty() ||
                ikcmp_->Compare(merger->key(), result->largest_key) > 0) {
                result->largest_key = merger->key();
            }
        }
    }
    
    result->compacted_n_entries = builder->NumEntries();
    return builder->error();
}
    
Error CompactionImpl::EnsureKeyNotExists(int start_level, std::string_view key,
                                         bool *drop) {
    for (int i = start_level; i < Config::kMaxLevel; ++i) {
        for (auto fmd : cfd()->current()->level_files(i)) {
            if (ikcmp_->Compare(key, fmd->largest_key) > 0 ||
                ikcmp_->Compare(key, fmd->smallest_key) < 0) {
                continue;
            }
            
            std::shared_ptr<core::KeyFilter> filter;
            Error rs = table_cache_->GetKeyFilter(cfd(), fmd->number, &filter);
            if (!rs) {
                *drop = false;
                return rs;
            }
            if (filter->MayExists(key)) {
                *drop = false;
                Error::OK();
            }
        }
    }
    *drop = true;
    return Error::OK();
}
    
} // namespace db

} // namespace mai


