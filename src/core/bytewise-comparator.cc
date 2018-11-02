#include "mai/comparator.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace core {
    
class BytewiseComparator final : public Comparator {
public:
    BytewiseComparator() {}
    virtual ~BytewiseComparator() {}
    
    virtual
    int Compare(std::string_view lhs, std::string_view rhs) const override {
        return lhs.compare(rhs);
    }
    
    virtual const char* Name() const override {
        return "::mai::BitwiseComparator";
    }
    
    virtual void FindShortestSeparator(std::string* start,
                                       std::string_view limit) const override {
        // Find length of common prefix
        size_t min_length = std::min(start->size(), limit.size());
        size_t diff_index = 0;
        while ((diff_index < min_length) &&
               ((*start)[diff_index] == limit[diff_index])) {
            diff_index++;
        }
        
        if (diff_index >= min_length) {
            // Do not shorten if one string is a prefix of the other
        } else {
            uint8_t start_byte = static_cast<uint8_t>((*start)[diff_index]);
            uint8_t limit_byte = static_cast<uint8_t>(limit[diff_index]);
            if (start_byte >= limit_byte) {
                // Cannot shorten since limit is smaller than start or start is
                // already the shortest possible.
                return;
            }
            assert(start_byte < limit_byte);
            
            if (diff_index < limit.size() - 1 || start_byte + 1 < limit_byte) {
                (*start)[diff_index]++;
                start->resize(diff_index + 1);
            } else {
                //     v
                // A A 1 A A A
                // A A 2
                //
                // Incrementing the current byte will make start bigger than limit, we
                // will skip this byte, and find the first non 0xFF byte in start and
                // increment it.
                diff_index++;
                
                while (diff_index < start->size()) {
                    // Keep moving until we find the first non 0xFF byte to
                    // increment it
                    if (static_cast<uint8_t>((*start)[diff_index]) <
                        static_cast<uint8_t>(0xff)) {
                        (*start)[diff_index]++;
                        start->resize(diff_index + 1);
                        break;
                    }
                    diff_index++;
                }
            }
            DCHECK_LT(Compare(*start, limit), 0);
        }
    }
    
    virtual void FindShortSuccessor(std::string* key) const override {
        // Find first character that can be incremented
        size_t n = key->size();
        for (size_t i = 0; i < n; i++) {
            const uint8_t byte = (*key)[i];
            if (byte != static_cast<uint8_t>(0xff)) {
                (*key)[i] = byte + 1;
                key->resize(i+1);
                return;
            }
        }
        // *key is a run of 0xffs.  Leave it alone.
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytewiseComparator);
}; // class BytewiseComparator
    
} // namespace core
    
/*static*/ const Comparator *Comparator::Bytewise() {
    static core::BytewiseComparator bcmp;
    return &bcmp;
}
    
} // namespace mai
