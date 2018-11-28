#include "table/plain-block-builder.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "mai/comparator.h"

namespace mai {
    
namespace table {
    
uint32_t PlainBlockBuilder::Add(std::string_view key, std::string_view value) {
    using ::mai::core::KeyBoundle;
    using ::mai::core::Tag;
    using ::mai::base::Slice;
    
    DCHECK(!finish_);
    
    uint32_t internal_offset = static_cast<uint32_t>(buf_.size());
    if (last_level_) {
        if (!last_key_.empty() && ucmp_->Equals(last_key_, key)) {
            // Shared key size;
            buf_.append(Slice::GetV64(last_key_.size(), &scope_));
            // Private key size:
            buf_.append(Slice::GetV64(0, &scope_));
        } else {
            // Shared key size;
            buf_.append(Slice::GetV64(0, &scope_));
            // Private key size:
            buf_.append(Slice::GetV64(key.size(), &scope_));
            // Key body
            buf_.append(key);
        }
    } else {
        std::string_view k1 = last_key_.empty() ? ""
            : KeyBoundle::ExtractUserKey(last_key_);
        std::string_view k2 = KeyBoundle::ExtractUserKey(key);
        if (!last_key_.empty() && ucmp_->Equals(k1, k2)) {
            // Shared key size;
            buf_.append(Slice::GetV64(k1.size(), &scope_));
            // Private key size:
            buf_.append(Slice::GetV64(key.size() - k2.size(), &scope_));
            // Key body
            buf_.append(key.substr(k2.size()));
        } else {
            // Shared key size;
            buf_.append(Slice::GetV64(0, &scope_));
            // Private key size:
            buf_.append(Slice::GetV64(key.size(), &scope_));
            // Key body
            buf_.append(key);
        }
    }
    buf_.append(Slice::GetV64(value.size(), &scope_));
    buf_.append(value);
    last_key_ = key;
    return internal_offset;
}

} // namespace table
    
} // namespace mai
