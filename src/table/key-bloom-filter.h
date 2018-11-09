#ifndef MAI_TABLE_KEY_BLOOM_FILTER_H_
#define MAI_TABLE_KEY_BLOOM_FILTER_H_

#include "core/key-filter.h"
#include "base/hash.h"
#include "base/base.h"

namespace mai {
    
namespace table {
    
class KeyBloomFilter final : public core::KeyFilter {
public:
    KeyBloomFilter(const uint32_t *buckets, size_t n_buckets,
                   const base::hash_func_t *hashs, size_t n_hash);
    virtual ~KeyBloomFilter();
    
    virtual bool MayExists(std::string_view key) const override;
    virtual bool EnsureNotExists(std::string_view key) const override;
    virtual size_t ApproximateCount() const override;
    virtual size_t memory_usage() const override;
    
    uint64_t n_bits() const { return n_buckets_ * 32; }
    uint64_t n_bytes() const { return n_buckets_ * 4; }
private:
    void set_bit(uint64_t i) { buckets_[i / 32] |= (1u << (i % 32)); }
    bool test_bit(uint64_t i) const {
        return buckets_[i / 32] & (1u << (i % 32));
    }
    
    const base::hash_func_t *hashs_;
    const size_t n_hashs_;
    
    std::unique_ptr<uint32_t[]> buckets_;
    size_t n_buckets_;
}; // class KeyBloomFilter
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_KEY_BLOOM_FILTER_H_
