#include "core/delta-amend-iterator.h"
#include "core/key-boundle.h"
#include "mai/comparator.h"

namespace mai {
    
namespace core {
    
/*virtual*/ DeltaAmendIterator::~DeltaAmendIterator() {
}

/*virtual*/ bool DeltaAmendIterator::Valid() const {
    return current_at_base_ ? base_iter_->Valid() : delta_iter_->Valid();
}

/*virtual*/ void DeltaAmendIterator::SeekToFirst() {
    direction_ = kForward;
    base_iter_->SeekToFirst();
    delta_iter_->SeekToFirst();
    UpdateCurrent();
}

/*virtual*/ void DeltaAmendIterator::SeekToLast() {
    direction_ = kReserve;
    base_iter_->SeekToLast();
    delta_iter_->SeekToLast();
    UpdateCurrent();
}
    
/*virtual*/ void DeltaAmendIterator::Seek(std::string_view target) {
    direction_ = kForward;
    base_iter_->Seek(target);
    delta_iter_->Seek(target);
    UpdateCurrent();
}

/*virtual*/ void DeltaAmendIterator::Next() {
    if (!Valid()) {
        error_ = MAI_NOT_SUPPORTED("Next() on invliad iterator.");
        return;
    }
    
    if (direction_ == kReserve) {
        direction_ = kForward;
        equal_keys_ = false;
        
        if (!base_iter_->Valid()) {
            DCHECK(delta_iter_->Valid());
            base_iter_->SeekToFirst();
        } else if (!delta_iter_->Valid()) {
            delta_iter_->SeekToFirst();
        } else if (current_at_base_) {
            AdvanceDelta();
        } else {
            AdvanceBase();
        }
        
        if (delta_iter_->Valid() && base_iter_->Valid()) {
            core::ParsedTaggedKey delta_key;
            core::KeyBoundle::ParseTaggedKey(delta_iter_->key(), &delta_key);
            if (cmp_->Equals(delta_key.user_key, base_iter_->key())) {
                equal_keys_ = true;
            }
        }
    }
    Advance();
}

/*virtual*/ void DeltaAmendIterator::Prev() {
    if (!Valid()) {
        error_ = MAI_NOT_SUPPORTED("Prev() on invliad iterator.");
        return;
    }
    if (direction_ == kForward) {
        direction_ = kReserve;
        equal_keys_ = false;
        
        if (!base_iter_->Valid()) {
            DCHECK(delta_iter_->Valid());
            base_iter_->SeekToLast();
        } else if (!delta_iter_->Valid()) {
            delta_iter_->SeekToLast();
        } else if (current_at_base_) {
            AdvanceDelta();
        } else {
            AdvanceBase();
        }
        
        if (delta_iter_->Valid() && base_iter_->Valid()) {
            core::ParsedTaggedKey delta_key;
            core::KeyBoundle::ParseTaggedKey(delta_iter_->key(), &delta_key);
            if (cmp_->Equals(delta_key.user_key, base_iter_->key())) {
                equal_keys_ = true;
            }
        }
    }
    Advance();
}

/*virtual*/ std::string_view DeltaAmendIterator::key() const {
    return current_at_base_ ? base_iter_->key() :
                            core::KeyBoundle::ExtractUserKey(delta_iter_->key());
}

/*virtual*/ std::string_view DeltaAmendIterator::value() const {
    return current_at_base_ ? base_iter_->value() : delta_iter_->value();
}

/*virtual*/ Error DeltaAmendIterator::error() const {
    if (base_iter_->error().fail()) {
        return base_iter_->error();
    }
    if (delta_iter_->error().fail()) {
        return delta_iter_->error();
    }
    return error_;
}
    
void DeltaAmendIterator::Advance() {
    if (equal_keys_) {
        DCHECK(base_iter_->Valid());
        DCHECK(delta_iter_->Valid());
        AdvanceBase();
        AdvanceDelta();
    } else {
        if (current_at_base_) {
            DCHECK(base_iter_->Valid());
            AdvanceBase();
        } else {
            DCHECK(delta_iter_->Valid());
            AdvanceDelta();
        }
    }
    UpdateCurrent();
}
    
void DeltaAmendIterator::UpdateCurrent() {
    error_ = Error::OK();

    while (true) {
        core::ParsedTaggedKey delta_key;

        if (delta_iter_->Valid()) {
            DCHECK(delta_iter_->error().ok());

            core::KeyBoundle::ParseTaggedKey(delta_iter_->key(), &delta_key);
        } else if (delta_iter_->error().fail()) {
            current_at_base_ = false;
            break;
        }
        
        equal_keys_ = false;
        if (!base_iter_->Valid()) {
            if (base_iter_->error().fail()) {
                current_at_base_ = true;
                break;
            }
            
            if (!delta_iter_->Valid()) {
                break;
            }
            
            if (delta_key.tag.flag() == core::Tag::kFlagDeletion) {
                AdvanceDelta();
            } else {
                current_at_base_ = false;
                break;
            }
        } else if (!delta_iter_->Valid()) {
            current_at_base_ = true;
            break;
        } else {
            int rv = (direction_ == kForward ? 1 : -1) *
                     cmp_->Compare(delta_key.user_key, base_iter_->key());
            if (rv <= 0) {
                if (rv == 0) {
                    equal_keys_ = true;
                }
                if (delta_key.tag.flag() != core::Tag::kFlagDeletion) {
                    current_at_base_ = false;
                    break;
                }
                AdvanceDelta();
                if (equal_keys_) {
                    AdvanceBase();
                }
            } else {
                current_at_base_ = true;
                break;
            }
        }
    }
    //Assert();
}
    
void DeltaAmendIterator::Assert() {
#if defined(DEBUG) || defined(_DEBUG)
    bool not_ok = false;
    if (base_iter_->error().fail()) {
        DCHECK(!base_iter_->Valid());
        not_ok = true;
    }
    if (delta_iter_->error().fail()) {
        DCHECK(!delta_iter_->Valid());
        not_ok = true;
    }
    if (not_ok) {
        DCHECK(!Valid());
        DCHECK(error().fail());
    }
    
    if (!Valid()) {
        return;
    }
    if (!base_iter_->Valid()) {
        DCHECK(!current_at_base_ && delta_iter_->Valid());
        return;
    }
    if (!delta_iter_->Valid()) {
        DCHECK(current_at_base_ && base_iter_->Valid());
        return;
    }
    
    core::ParsedTaggedKey delta_key;
    core::KeyBoundle::ParseTaggedKey(delta_iter_->key(), &delta_key);
    DCHECK_NE(delta_key.tag.flag(), core::Tag::kFlagValueForSeek);
    
    int rv = cmp_->Compare(delta_key.user_key, base_iter_->key());
    if (direction_ == kForward) {
        DCHECK(!current_at_base_ || rv < 0);
        DCHECK(current_at_base_ && rv >= 0);
    } else {
        DCHECK(!current_at_base_ || rv > 0);
        DCHECK(current_at_base_ && rv <= 0);
    }
    DCHECK((equal_keys_ || rv != 0) && (!equal_keys_ || rv == 0));
#endif
}

    
} // namespace core
    
} // namespace mai
