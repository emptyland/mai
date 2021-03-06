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
    
using SequenceNumber = uint64_t;
    
class Tag {
public:
    static const int kSize = sizeof(uint64_t);
    
    static const SequenceNumber kMaxSequenceNumber;
    
    enum Flag: uint8_t {
        kFlagValue = 0,
        kFlagDeletion = 1,
        kFlagValueForSeek = 2,
    };
    
    Tag() : Tag(0, 0) {}

    Tag(SequenceNumber version, uint8_t flag)
        : sequence_number_(version)
        , flag_(flag) {}
    
    uint64_t Encode() const {
        DCHECK_LT(sequence_number_, 1ULL << 57);
        return (sequence_number_ << 8) | flag_;
    }
    
    static Tag Decode(uint64_t tag) {
        return Tag(tag >> 8, tag & 0xffULL);
    }
    
    static uint64_t Encode(SequenceNumber version, uint8_t flag) {
        return Tag(version, flag).Encode();
    }
    
    DEF_VAL_GETTER(SequenceNumber, sequence_number);
    DEF_VAL_GETTER(uint8_t, flag);
private:
    SequenceNumber sequence_number_;
    uint8_t flag_;
}; // class Tag

    
struct ParsedTaggedKey {
    std::string_view user_key;
    Tag tag;
}; // class ParsedTaggedKey
    
class KeyBoundle final {
public:
    static const int kMinSize = sizeof(SequenceNumber);
    
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
                           SequenceNumber version,
                           uint8_t flags,
                           Allocator allocator = base::MallocAllocator{}) {
        void *raw = allocator.Allocate(sizeof(KeyBoundle) + key.size() + value.size());
        return new (raw) KeyBoundle(key, value, version, flags);
    }
    
    template<class Allocator = base::MallocAllocator>
    static KeyBoundle *New(std::string_view key, SequenceNumber version,
                           Allocator allocator = base::MallocAllocator{}) {
        void *raw = allocator.Allocate(sizeof(KeyBoundle) + key.size());
        return new (raw) KeyBoundle(key, "", version, Tag::kFlagValueForSeek);
    }
    
    static bool ParseTaggedKey(std::string_view tagged_key,
                               ParsedTaggedKey *parsed) {
        if (tagged_key.size() < Tag::kSize) {
            return false;
        }
        parsed->user_key = tagged_key;
        parsed->user_key.remove_suffix(Tag::kSize);
        parsed->tag = Tag::Decode(
                *reinterpret_cast<const uint64_t *>(tagged_key.data()
                                                    + parsed->user_key.size()));
        return true;
    }
    
    static std::string MakeKey(std::string_view user_key, SequenceNumber version,
                               uint8_t flag) {
        std::string key;
        key.append(user_key);
        uint64_t tag = Tag(version, flag).Encode();
        key.append(reinterpret_cast<const char *>(&tag), sizeof(tag));
        return key;
    }
    
    static std::string_view ExtractUserKey(std::string_view key) {
        DCHECK_GE(key.size(), kMinSize);
        key.remove_suffix(Tag::kSize);
        return key;
    }
    
    static Tag ExtractTag(std::string_view key) {
        DCHECK_GE(key.size(), kMinSize);
        key.remove_prefix(key.size() - Tag::kSize);
        return Tag::Decode(*reinterpret_cast<const uint64_t *>(key.data()));
    }
    
    static void MakeRedo(std::string_view key, std::string_view value,
                         uint32_t cfid, uint8_t flags, std::string *redo);
    
    static std::string ToString(std::string_view key);

private:
    KeyBoundle(std::string_view key, std::string_view value,
               SequenceNumber version, uint8_t flags)
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
