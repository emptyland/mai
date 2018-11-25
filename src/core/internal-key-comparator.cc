#include "core/internal-key-comparator.h"
#include "core/key-boundle.h"

namespace mai {
    
namespace core {
    
/*virtual*/ InternalKeyComparator::~InternalKeyComparator() {}

/*virtual*/ int InternalKeyComparator::Compare(std::string_view lhs,
                    std::string_view rhs) const {
    ParsedTaggedKey lkey, rkey;
    KeyBoundle::ParseTaggedKey(lhs, &lkey);
    KeyBoundle::ParseTaggedKey(rhs, &rkey);
    
    int rv = ucmp_->Compare(lkey.user_key, rkey.user_key);
    if (rv != 0) {
        return rv;
    }
    if (lkey.tag.sequence_number() < rkey.tag.sequence_number()) {
        return 1;
    } else if (lkey.tag.sequence_number() > rkey.tag.sequence_number()) {
        return -1;
    }
    return 0;
}

/*virtual*/ uint32_t InternalKeyComparator::Hash(std::string_view value) const {
    return ucmp_->Hash(KeyBoundle::ExtractUserKey(value));
}

/*virtual*/ const char* InternalKeyComparator::Name() const {
    return "::mai::core::InternalKeyComparator";
}

/*virtual*/
void InternalKeyComparator::FindShortestSeparator(std::string* start,
                                                  std::string_view limit) const {
    // TODO:
    ucmp_->FindShortestSeparator(start, limit);
}

/*virtual*/
void InternalKeyComparator::FindShortSuccessor(std::string* key) const {
    // TODO:
    ucmp_->FindShortSuccessor(key);
}
    
} // namespace core
    
} // namespace mai
