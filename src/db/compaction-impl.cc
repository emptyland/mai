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
    
using ::mai::core::Tag;
using ::mai::core::KeyBoundle;
using ::mai::core::ParsedTaggedKey;
using ::mai::core::Merging;
using ::mai::core::SequenceNumber;
    
    
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
        merger(Merging::NewMergingIterator(ikcmp_,
                                           mutable_original_input()->data(),
                                           original_input().size()));
    if (merger->error().fail()) {
        return merger->error();
    }
    mutable_original_input()->clear();
    merger->SeekToFirst();

    bool to_last_level = (target_level() == (Config::kMaxLevel - 1));
    std::string current_user_key;
    bool has_current_user_key = false;
    SequenceNumber last_sequence_for_key = Tag::kMaxSequenceNumber;
    ParsedTaggedKey ikey;
    for (;merger->Valid(); merger->Next()) {
        KeyBoundle::ParseTaggedKey(merger->key(), &ikey);
        
        bool drop = false;

        if (!has_current_user_key ||
            !ikcmp_->ucmp()->Equals(current_user_key, ikey.user_key)) {

            // First occurrence of this user key
            current_user_key.assign(ikey.user_key.data(), ikey.user_key.size());
            has_current_user_key = true;
            last_sequence_for_key = Tag::kMaxSequenceNumber;
        }

        if (last_sequence_for_key <= smallest_snapshot()) {
            drop = true;
        } else if (ikey.tag.flag() == Tag::kFlagValue &&
                   ikey.tag.sequence_number() <= smallest_snapshot()) {
            // TODO:
            // If key flag is value and has new version in memory table,
            // Can drop it.
        } else if (ikey.tag.flag() == Tag::kFlagDeletion &&
            ikey.tag.sequence_number() <= smallest_snapshot()) {
            // If key flag is deletion and has no oldest versions,
            // Can drop it.
            
            bool key_may_exists;
            Error rs = IsBaseLevelForKey(target_level() + 1, merger->key(),
                                         &key_may_exists);
            if (!rs) {
                return rs;
            }
            drop = key_may_exists;
        }
        last_sequence_for_key = ikey.tag.sequence_number();

        if (to_last_level) {
            if (drop) {
                result->deletion_keys++;
                result->deletion_size += merger->key().size();
                result->deletion_size += merger->value().size();
            } else {
                std::string key = KeyBoundle::MakeKey(ikey.user_key, 0,
                                                      Tag::kFlagValue);
                builder->Add(key, merger->value());
                result->compacted_size += (key.size() + merger->value().size());
            }
        } else {
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
    
Error CompactionImpl::IsBaseLevelForKey(int start_level, std::string_view key,
                                        bool *may_exists) {
    *may_exists = false;
    
    if (start_level == Config::kMaxLevel - 1) {
        return Error::OK();
    }
    for (int i = start_level; i < Config::kMaxLevel; ++i) {
        for (auto fmd : input_version()->level_files(i)) {
            if (ikcmp_->Compare(key, fmd->largest_key) > 0 ||
                ikcmp_->Compare(key, fmd->smallest_key) < 0) {
                continue;
            }
            
            base::intrusive_ptr<core::KeyFilter> filter;
            Error rs = table_cache_->GetKeyFilter(cfd(), fmd->number, &filter);
            if (!rs) {
                *may_exists = true;
                return rs;
            }
            if (filter->MayExists(key)) {
                *may_exists = true;
                Error::OK();
            }
        }
    }
    return Error::OK();
}
    
} // namespace db

} // namespace mai


