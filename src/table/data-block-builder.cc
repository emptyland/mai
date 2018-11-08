#include "table/data-block-builder.h"

namespace mai {
    
namespace table {

    
void DataBlockBuilder::Add(std::string_view key, std::string_view value) {
    using ::mai::base::Slice;
    
    DCHECK(!has_finish_);
    base::ScopedMemory scope;
    if (count_ == 0) {
        DCHECK_LT(buf_.size(), 0xffffffff);
        restarts_.push_back(static_cast<uint32_t>(buf_.size()));
        
        // shared-size
        buf_.append(Slice::GetV64(0, &scope));
        // private-size
        buf_.append(Slice::GetV64(key.size(), &scope));
        // key
        buf_.append(key);
    } else {
        
        size_t shared_size = ExtractPrefix(key);
        // shared-size
        buf_.append(Slice::GetV64(shared_size, &scope));
        // private-size
        buf_.append(Slice::GetV64(key.size() - shared_size, &scope));
        // key
        buf_.append(key.substr(shared_size));
    }
    
    buf_.append(Slice::GetV64(value.size(), &scope));
    buf_.append(value);
    last_key_ = key;
    count_ = (count_ + 1) % n_restart_;
}

} // namespace table
    
} // namespace mai
