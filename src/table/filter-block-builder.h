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
    static const size_t kBloomFilterSizeLimit = 10 * base::kMB;
    
    FilterBlockBuilder(const uint64_t data_bytes,
                       const base::hash_func_t *hashs, size_t n)
        : data_bytes_(data_bytes)
        , hashs_(DCHECK_NOTNULL(hashs))
        , n_hashs_(n) {
        DCHECK_EQ(0, data_bytes_ % sizeof(uint32_t));
        bits_.reset(new uint32_t[data_bytes/sizeof(uint32_t)]);
        Reset();
    }
    
    void Reset() { memset(bits_.get(), 0, data_bytes_); }
    
    void AddKey(std::string_view key) {
        for (size_t i = 0; i < n_hashs_; ++i) {
            set_bit(hashs_[i](key.data(), key.size()) % n_bits());
        }
    }

    std::string_view Finish() {
        return std::string_view(reinterpret_cast<const char *>(bits_.get()),
                                data_bytes_);
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
    
    uint64_t n_bits() const { return data_bytes_ * 8; }
    
    uint64_t n_bytes() const { return data_bytes_; }
    
    static size_t ComputeBoomFilterSize(size_t approximated_n_entries,
                                        size_t alignment,
                                        size_t bits) {
        if (approximated_n_entries == 0) {
            return alignment * 2;
        }
        size_t result = (approximated_n_entries * (bits * 2) + 7) / 8;
        result = RoundUp(result, alignment) + alignment;
        return result;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FilterBlockBuilder);
private:
    void set_bit(uint64_t i) { bits_[i / 32] |= (1u << (i % 32)); }
    bool test_bit(uint64_t i) const {
        return bits_[i / 32] & (1u << (i % 32));
    }
    
    const uint64_t data_bytes_;
    const base::hash_func_t *hashs_;
    const size_t n_hashs_;
    
    std::unique_ptr<uint32_t[]> bits_;
}; // class FilterBlockBuilder
    
} // namespace table

} // namespace mai


#endif // MAI_TABLE_FILTER_BLOCK_BUILDER_H_
