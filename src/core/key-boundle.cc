#include "core/key-boundle.h"
#include "base/slice.h"

namespace mai {
    
namespace core {
    
/*static*/ void KeyBoundle::MakeRedo(std::string_view key, std::string_view value,
                                     uint32_t cfid, uint8_t flag,
                                     std::string *redo) {
    using ::mai::base::ScopedMemory;
    using ::mai::base::Slice;
    
    ScopedMemory scope;
    redo->append(1, flag);
    
    redo->append(Slice::GetV32(cfid, &scope));
    redo->append(Slice::GetV64(key.size(), &scope));
    redo->append(key);
    
    if (flag != Tag::kFlagDeletion) {
        redo->append(Slice::GetV64(value.size(), &scope));
        redo->append(value);
    }
}
    
} // namespace core
    
} // namespace mai
