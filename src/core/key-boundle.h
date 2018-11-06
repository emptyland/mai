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
    
    Tag() : Tag(0, 0) {}

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
    Version version_;
    uint8_t flags_;
}; // class Tag

    
struct ParsedTaggedKey {
    std::string_view user_key;
    Tag tag;
}; // class ParsedTaggedKey
    
class KeyBoundle final {
public:
    static const int kMinSize = sizeof(Version);
    
    size_t size() const { return size_; }
    size_t user_key_size() const { return user_key_size_; }
    size_t key_size() const { return user_key_size_ + Tag::kSize; }
    size_t value_size() const { return size_ - user_key_size_ - Tag::kSize; }
    
    // user key
    std::string_view user_key() const {
        return std::string_view(key_, user_key_size());
    }
    
    // user key with tag
    std::string_view key() const {
        return std::string_view(key_, key_size());
    }
    
    std::string_view value() const {
        return std::string_view(key_ + user_key_size() + Tag::kSize, value_size());
    }
    
    Tag tag() const {
        return Tag::Decode(*reinterpret_cast<const uint64_t *>(key_
                                                               + user_key_size()));
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
    
    static void ParseTaggedKey(std::string_view tagged_key,
                               ParsedTaggedKey *parsed) {
        parsed->user_key = tagged_key;
        parsed->user_key.remove_suffix(Tag::kSize);
        parsed->tag = Tag::Decode(
                *reinterpret_cast<const uint64_t *>(tagged_key.data()
                                                    + parsed->user_key.size()));
    }
    
    static std::string_view ExtractUserKey(std::string_view key) {
        DCHECK_GE(key.size(), kMinSize);
        key.remove_suffix(Tag::kSize);
        return key;
    }

private:
    KeyBoundle(std::string_view key, std::string_view value,
               Version version, uint8_t flags)
        : size_(key.size() + value.size() + Tag::kSize)
        , user_key_size_(static_cast<int>(key.size())) {
        ::memcpy(key_, key.data(), key.size());
        uint64_t tag = Tag(version, flags).Encode();
        *reinterpret_cast<uint64_t *>(key_ + user_key_size_) = tag;
        ::memcpy(key_ + user_key_size_ + Tag::kSize, value.data(), value.size());
    }
    
    const uint64_t size_;
    const uint32_t user_key_size_;
    char           key_[Tag::kSize];
}; // class KeyBoundle
    
} // namespace core
    
} // namespace mai


#endif // MAI_CORE_KEY_BOUNDLE_H_
