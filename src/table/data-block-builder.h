#ifndef MAI_TABLE_DATA_BLOCK_BUILDER_H_
#define MAI_TABLE_DATA_BLOCK_BUILDER_H_

#include "base/slice.h"
#include "base/base.h"
#include <string>
#include <string_view>
#include <vector>

namespace mai {
    
namespace table {

class DataBlockBuilder final {
public:
    DataBlockBuilder(int n_restart) : n_restart_(n_restart) {}
    
    void Reset() {
        buf_.clear();
        restarts_.clear();
        last_key_.clear();
        count_ = 0;
        has_finish_ = false;
    }
    
    void Add(std::string_view key, std::string_view value);
    
    std::string_view Finish() {
        using ::mai::base::Slice;
        
        if (buf_.empty()) {
            return "";
        }
        base::ScopedMemory scope;
        for (uint32_t offset : restarts_) {
            buf_.append(Slice::GetU32(offset, &scope));
        }
        buf_.append(Slice::GetU32(static_cast<uint32_t>(restarts_.size()),
                                  &scope));
        has_finish_ = true;
        return buf_;
    }
    
    size_t CurrentSizeEstimate() const {
        return buf_.size() + restarts_.size() * sizeof(uint32_t)
            + sizeof(uint32_t);
    }
    
    DEF_VAL_GETTER(bool, has_finish);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DataBlockBuilder);
private:
    const int n_restart_;
    
    size_t ExtractPrefix(std::string_view input) const {
        size_t n = std::min(input.size(), last_key_.size());
        for (int i = 0; i < n; ++i) {
            if (last_key_[i] != input[i]) {
                return i;
            }
        }
        return n;
    }
    
    std::string buf_;
    std::vector<uint32_t> restarts_;
    std::string last_key_;
    int count_ = 0;
    bool has_finish_ = false;
}; // class DataBlockBuilder

} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_DATA_BLOCK_BUILDER_H_
