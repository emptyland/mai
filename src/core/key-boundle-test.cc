#include "core/key-boundle.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace core {
    
TEST(KeyBoundleTest, New) {
    auto boundle = KeyBoundle::New("key", "value", 1, Tag::kFlagValue);
    ASSERT_EQ(16, boundle->size());
    ASSERT_EQ(3, boundle->key_size());
    ASSERT_EQ(5, boundle->value_size());
    
    ASSERT_EQ(0, ::strncmp("key", boundle->key().data(),
                           boundle->key().size()));
    ASSERT_EQ(0, ::strncmp("value", boundle->value().data(),
                           boundle->value().size()));
    
    auto tag = boundle->tag();
    ASSERT_EQ(1, tag.version());
    ASSERT_EQ(Tag::kFlagValue, tag.flags());
}
    
TEST(KeyBoundleTest, TempAllocate) {
    auto boundle = KeyBoundle::New("100", 0, base::NewAllocator{});
    ASSERT_NE(nullptr, boundle);
    base::NewAllocator{}.Free(boundle);
    
    base::ScopedMemory scope_memory;
    
    boundle = KeyBoundle::New("100", 0, base::ScopedAllocator{&scope_memory});
    ASSERT_NE(nullptr, boundle);
}
    
} // namespace core
    
} // namespace mai
