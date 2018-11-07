#include "core/iterator-warpper.h"
#include "glog/logging.h"

namespace mai {
    
namespace core {
    
IteratorWarpper::~IteratorWarpper() {}

/*virtual*/ bool IteratorWarpper::Valid() const { return valid_; }

/*virtual*/ void IteratorWarpper::SeekToFirst() {
    delegated_->SeekToFirst();
    Update();
}

/*virtual*/ void IteratorWarpper::SeekToLast() {
    delegated_->SeekToLast();
    Update();
}

/*virtual*/ void IteratorWarpper::Seek(std::string_view target) {
    delegated_->Seek(target);
    Update();
}

/*virtual*/ void IteratorWarpper::Next() {
    delegated_->Next();
    Update();
}

/*virtual*/ void IteratorWarpper::Prev() {
    delegated_->Prev();
    Update();
}

/*virtual*/ std::string_view IteratorWarpper::key() const {
    DCHECK(Valid());
    return key_;
}

/*virtual*/ std::string_view IteratorWarpper::value() const {
    return delegated_->value();
}

/*virtual*/ Error IteratorWarpper::error() const {
    return delegated_->error();
}

} // namespace core
    
} // namespace mai
