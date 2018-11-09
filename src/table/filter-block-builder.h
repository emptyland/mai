#ifndef MAI_TABLE_FILTER_BLOCK_BUILDER_H_
#define MAI_TABLE_FILTER_BLOCK_BUILDER_H_

#include "base/hash.h"
#include "base/base.h"
#include "glog/logging.h"
#include <string>
#include <string_view>

namespace mai {
    
namespace table {
    
class FilterBlockBuilder final {
public:
    FilterBlockBuilder(const uint64_t block_size,
                       const base::hash_func_t *hashs, size_t n)
        : block_size_(block_size)
        , hashs_(new base::hash_func_t[n])
        , n_hashs_(n) {
            
        for (size_t i = 0; i < n; ++i) {
            hashs_[i] = hashs[i];
        }
        DCHECK_EQ(0, block_size_ % sizeof(uint32_t));
        bits_.reset(new uint32_t[block_size/sizeof(uint32_t)]);
        Reset();
    }
    
    void Reset() { memset(bits_.get(), 0, block_size_); }
    
    void AddKey(std::string_view key) {
        for (size_t i = 0; i < n_hashs_; ++i) {
            set_bit(hashs_[i](key.data(), key.size()) % n_bits());
        }
    }

    std::string_view Finish() {
        return std::string_view(reinterpret_cast<const char *>(bits_.get()),
                                block_size_);
    }
    
    bool MayExist(std::string_view key) const {
        for (size_t i = 0; i < n_hashs_; ++i) {
            if (!test_bit(hashs_[i](key.data(), key.size()) % n_bits())) {
                return false;
            }
        }
        return true;
    }
    
    bool EnsureNotExist(std::string_view key) const { return !MayExist(key); }
    
    uint64_t n_bits() const { return block_size_ * 8; }
    
    uint64_t n_bytes() const { return block_size_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FilterBlockBuilder);
private:
    void set_bit(uint64_t i) { bits_[i / 32] |= (1u << (i % 32)); }
    bool test_bit(uint64_t i) const {
        return bits_[i / 32] & (1u << (i % 32));
    }
    
    const uint64_t block_size_;
    std::unique_ptr<base::hash_func_t[]> hashs_;
    const size_t n_hashs_;
    
    std::unique_ptr<uint32_t[]> bits_;
}; // class FilterBlockBuilder
    
} // namespace table

} // namespace mai


#endif // MAI_TABLE_FILTER_BLOCK_BUILDER_H_
