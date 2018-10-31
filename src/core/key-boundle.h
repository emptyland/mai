#ifndef MAI_CORE_KEY_BOUNDLE_H_
#define MAI_CORE_KEY_BOUNDLE_H_

#include "base/allocators.h"
#include "base/base.h"
#include "glog/logging.h"
#include <stdint.h>
#include <string.h>
#include <string_view>

namespace mai {
    
namespace core {
    
typedef uint64_t Version;
    
class Tag {
public:
    static const int kSize = sizeof(uint64_t);
    
    enum Flag: uint8_t {
        kFlagValue = 0,
        kFlagDeletion = 1
    };

    Tag(Version version, uint8_t flags)
        : version_(version)
        , flags_(flags) {}
    
    uint64_t Encode() const {
        DCHECK_LT(version_, 1ULL << 57);
        return (version_ << 8) | flags_;
    }
    
    static Tag Decode(uint64_t tag) {
        return Tag(tag >> 8, tag & 0xffULL);
    }
    
    DEF_VAL_GETTER(Version, version);
    DEF_VAL_GETTER(uint8_t, flags);
private:
    const Version version_;
    const uint8_t flags_;
}; // class Tag

    
class KeyBoundle final {
public:
    size_t size() const { return size_; }
    size_t key_size() const { return key_size_; }
    size_t tagged_key_size() const { return key_size_ + Tag::kSize; }
    size_t value_size() const { return size_ - key_size_ - Tag::kSize; }
    
    // user key
    std::string_view key() const {
        return std::string_view(key_, key_size());
    }
    
    // user key with tag
    std::string_view tagged_key() const {
        return std::string_view(key_, tagged_key_size());
    }
    
    std::string_view value() const {
        return std::string_view(key_ + key_size() + Tag::kSize, value_size());
    }
    
    Tag tag() const {
        return Tag::Decode(*reinterpret_cast<const uint64_t *>(key_
                                                               + key_size()));
    }
    
    template<class Allocator = base::MallocAllocator>
    static KeyBoundle *New(std::string_view key,
                           std::string_view value,
                           Version version,
                           uint8_t flags,
                           Allocator allocator = base::MallocAllocator{}) {
        void *raw = allocator.Allocate(sizeof(KeyBoundle) + key.size() + value.size());
        return new (raw) KeyBoundle(key, value, version, flags);
    }
    
    template<class Allocator = base::MallocAllocator>
    static KeyBoundle *New(std::string_view key, Version version,
                           Allocator allocator = base::MallocAllocator{}) {
        void *raw = allocator.Allocate(sizeof(KeyBoundle) + key.size());
        return new (raw) KeyBoundle(key, "", version, 0);
    }

private:
    KeyBoundle(std::string_view key, std::string_view value,
               Version version, uint8_t flags)
        : size_(key.size() + value.size() + Tag::kSize)
        , key_size_(static_cast<int>(key.size())) {
        ::memcpy(key_, key.data(), key.size());
        uint64_t tag = Tag(version, flags).Encode();
        *reinterpret_cast<uint64_t *>(key_ + key_size_) = tag;
        ::memcpy(key_ + key_size_ + Tag::kSize, value.data(), value.size());
    }
    
    const uint64_t size_;
    const uint32_t key_size_;
    char           key_[Tag::kSize];
}; // class KeyBoundle
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_KEY_BOUNDLE_H_
