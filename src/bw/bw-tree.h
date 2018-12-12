#ifndef MAI_BW_BW_TREE_H_
#define MAI_BW_BW_TREE_H_

#include "bw/bw-nodes.h"
#include "mai/iterator.h"

namespace mai {
struct ReadOptions;
namespace bw {
    
class BwTree final {
public:
    
    Error Put(std::string_view key, std::string_view value,
              core::SequenceNumber version, uint8_t flag);
    
    Error Get(std::string_view key, core::SequenceNumber version, core::Tag *tag,
              std::string *value);
    
    Iterator *NewIterator(const ReadOptions &options);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BwTree);
}; // class BwTree
    
    
} // namespace bw

} // namespace mai

#endif // MAI_BW_BW_TREE_H_
