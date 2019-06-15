#include "core/key-boundle.h"
#include "base/slice.h"

namespace mai {
    
namespace core {
    
/*static*/ const SequenceNumber Tag::kMaxSequenceNumber = 0x00ffffffffffffffULL;
    
using ::mai::base::Slice;
using ::mai::base::ScopedMemory;
    
/*static*/ void KeyBoundle::MakeRedo(std::string_view key,
                                     std::string_view value,
                                     uint32_t cfid, uint8_t flag,
                                     std::string *redo) {
    Slice::WriteVarint32(redo, cfid);
    redo->append(1, flag);

    Slice::WriteVarint64(redo, key.size());
    redo->append(key);

    if (flag != Tag::kFlagDeletion) {
        Slice::WriteVarint64(redo, value.size());
        redo->append(value);
    }
}
    
/*static*/ std::string KeyBoundle::ToString(std::string_view key) {
    ParsedTaggedKey ikey;
    if (!ParseTaggedKey(key, &ikey)) {
        return "ERROR!";
    }
    std::string buf;
    
    buf.append(Slice::ToReadable(ikey.user_key));
    buf.append(base::Sprintf("(%" PRIu64 ")", ikey.tag.sequence_number()));
    
    return buf;
}
    
} // namespace core
    
} // namespace mai
