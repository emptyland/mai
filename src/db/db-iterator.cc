#include "db/db-iterator.h"
#include "core/key-boundle.h"
#include "mai/comparator.h"

namespace mai {
    
namespace db {
    
/*virtual*/ DBIterator::~DBIterator() {}

/*virtual*/ bool DBIterator::Valid() const {
    return valid_;
}

/*virtual*/ void DBIterator::SeekToFirst() {
    direction_ = kForward;
    ClearSavedValue();
    iter_->SeekToFirst();
    if (iter_->Valid()) {
        FindNextUserEntry(false, &saved_key_ /* temporary storage */);
    } else {
        valid_ = false;
    }
}

/*virtual*/ void DBIterator::SeekToLast() {
    direction_ = kReserve;
    ClearSavedValue();
    iter_->SeekToLast();
    FindPrevUserEntry();
}

/*virtual*/ void DBIterator::Seek(std::string_view target) {
    direction_ = kForward;
    ClearSavedValue();
    saved_key_.clear();
    
    std::string key = core::KeyBoundle::MakeKey(target, last_sequence_number_,
                                                core::Tag::kFlagValueForSeek);
    iter_->Seek(key);
    if (iter_->Valid()) {
        FindNextUserEntry(false, &saved_key_ /* temporary storage */);
    } else {
        valid_ = false;
    }
}

/*virtual*/ void DBIterator::Next() {
    DCHECK(Valid());
    
    if (direction_ == kReserve) {  // Switch directions?
        direction_ = kForward;
        // iter_ is pointing just before the entries for this->key(),
        // so advance into the range of entries for this->key() and then
        // use the normal skipping code below.
        if (!iter_->Valid()) {
            iter_->SeekToFirst();
        } else {
            iter_->Next();
        }
        if (!iter_->Valid()) {
            valid_ = false;
            saved_key_.clear();
            return;
        }
        // saved_key_ already contains the key to skip past.
    } else {
        // Store in saved_key_ the current key so we skip it below.
        SaveKey(core::KeyBoundle::ExtractUserKey(iter_->key()), &saved_key_);
    }
    FindNextUserEntry(true, &saved_key_);
}
    
/*virtual*/ void DBIterator::Prev() {
    DCHECK(Valid());
    
    if (direction_ == kForward) {  // Switch directions?
        // iter_ is pointing at the current entry.  Scan backwards until
        // the key changes so we can use the normal reverse scanning code.
        DCHECK(iter_->Valid());  // Otherwise valid_ would have been false
        SaveKey(core::KeyBoundle::ExtractUserKey(iter_->key()), &saved_key_);
        while (true) {
            iter_->Prev();
            if (!iter_->Valid()) {
                valid_ = false;
                saved_key_.clear();
                ClearSavedValue();
                return;
            }
            if (ucmp_->Compare(core::KeyBoundle::ExtractUserKey(iter_->key()),
                              saved_key_) < 0) {
                break;
            }
        }
        direction_ = kReserve;
    }
    
    FindPrevUserEntry();
}

/*virtual*/ std::string_view DBIterator::key() const {
    DCHECK(Valid());
    return direction_ == kForward ?
        core::KeyBoundle::ExtractUserKey(iter_->key()) : saved_key_;
}

/*virtual*/ std::string_view DBIterator::value() const {
    DCHECK(Valid());
    return direction_ == kForward ? iter_->value() : saved_value_;
}

/*virtual*/ Error DBIterator::error() const {
    return error_.ok() ? iter_->error() : error_;
}
    
void DBIterator::FindNextUserEntry(bool skipping, std::string *skip) {
    // Loop until we hit an acceptable entry to yield
    DCHECK(iter_->Valid());
    DCHECK(direction_ == kForward);
    
    core::ParsedTaggedKey ikey;
    do {
        bool ok = core::KeyBoundle::ParseTaggedKey(iter_->key(), &ikey);
        DCHECK(ok) << "Incorrect internal key."; (void)ok;
        
        if (ikey.tag.sequence_number() <= last_sequence_number_) {
            switch (ikey.tag.flag()) {
                case core::Tag::kFlagDeletion:
                    // Arrange to skip all upcoming entries for this key since
                    // they are hidden by this deletion.
                    skip->assign(ikey.user_key.data(), ikey.user_key.size());
                    skipping = true;
                    break;
                    
                case core::Tag::kFlagValue:
                    if (skipping &&
                        ucmp_->Compare(ikey.user_key, *skip) <= 0) {
                        // Entry hidden
                    } else {
                        valid_ = true;
                        saved_key_.clear();
                        return;
                    }
                    break;
                    
                default:
                    DCHECK(false) << "noreached";
                    break;
            }
        }
        iter_->Next();
    } while (iter_->Valid());
    saved_key_.clear();
    valid_ = false;
}
    
void DBIterator::FindPrevUserEntry() {
    DCHECK(direction_ == kReserve);
    
    uint8_t value_type = core::Tag::kFlagDeletion;
    if (iter_->Valid()) {
        core::ParsedTaggedKey ikey;
        do {
            core::KeyBoundle::ParseTaggedKey(iter_->key(), &ikey);
            
            if (ikey.tag.sequence_number() <= last_sequence_number_) {
                if ((value_type != core::Tag::kFlagDeletion) &&
                    ucmp_->Compare(ikey.user_key, saved_key_) < 0) {
                    // We encountered a non-deleted value in entries for previous keys,
                    break;
                }
                value_type = ikey.tag.flag();
                if (value_type == core::Tag::kFlagDeletion) {
                    saved_key_.clear();
                    ClearSavedValue();
                } else {
                    std::string_view raw_value = iter_->value();
                    if (saved_value_.capacity() > raw_value.size() + 1048576) {
                        std::string empty;
                        swap(empty, saved_value_);
                    }
                    SaveKey(core::KeyBoundle::ExtractUserKey(iter_->key()),
                            &saved_key_);
                    saved_value_.assign(raw_value.data(), raw_value.size());
                }
            }
            iter_->Prev();
        } while (iter_->Valid());
    }
    
    if (value_type == core::Tag::kFlagDeletion) {
        // End
        valid_ = false;
        saved_key_.clear();
        ClearSavedValue();
        direction_ = kForward;
    } else {
        valid_ = true;
    }
}

} // namespace db
    
} // namespace mai
