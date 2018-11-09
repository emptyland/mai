#include "table/key-bloom-filter.h"
#include "base/bit-ops.h"
#include "glog/logging.h"

namespace mai {
    
namespace table {
    
KeyBloomFilter::KeyBloomFilter(const uint32_t *buckets, size_t n_buckets,
                               const base::hash_func_t *hashs, size_t n_hashs)
    : buckets_(new uint32_t[n_buckets])
    , n_buckets_(n_buckets)
    , hashs_(DCHECK_NOTNULL(hashs))
    , n_hashs_(n_hashs) {
    DCHECK_NOTNULL(buckets);
    DCHECK_GT(n_buckets_, 0);

    memcpy(buckets_.get(), buckets, n_buckets * sizeof(uint32_t));
}

/*virtual*/ KeyBloomFilter::~KeyBloomFilter() {}

/*virtual*/ bool KeyBloomFilter::MayExists(std::string_view key) const {
    for (size_t i = 0; i < n_hashs_; ++i) {
        uint32_t hash_val = hashs_[i](key.data(), key.size());
        if (!test_bit(hash_val % n_bits())) {
            return false;
        }
    }
    return true;
}

/*virtual*/ bool KeyBloomFilter::EnsureNotExists(std::string_view key) const {
    for (size_t i = 0; i < n_hashs_; ++i) {
        uint32_t hash_val = hashs_[i](key.data(), key.size());
        if (!test_bit(hash_val % n_bits())) {
            return true;
        }
    }
    return false;
}

/*virtual*/ size_t KeyBloomFilter::ApproximateCount() const {
    size_t count = 0;
    for (size_t i = 0; i < n_buckets_; ++i) {
        count += base::Bits::CountOne32(buckets_[i]);
    }
    return count / n_hashs_;
}

/*virtual*/ size_t KeyBloomFilter::memory_usage() const {
    return sizeof(*this) + n_buckets_ * sizeof(uint32_t);
}
    
} // namespace table
    
} // namespace mai
