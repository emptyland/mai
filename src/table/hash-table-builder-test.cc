#include "table/hash-table-builder.h"
#include "core/key-boundle.h"
#include "base/hash.h"
#include "base/allocators.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace table {

TEST(HashTableBuilderTest, Sanity) {
    auto env = Env::Default();
    
    std::unique_ptr<WritableFile> file;
    auto rs = env->NewWritableFile("tests/02-hash-table-file.tmp", &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    base::ScopedMemory scope;
    HashTableBuilder builder(file.get(), 103, &base::Hash::Js, 512);
    auto key = core::KeyBoundle::New("aaaa", "a11111", 1, core::Tag::kFlagValue,
                                     base::ScopedAllocator{&scope});
    builder.Add(key->tagged_key(), key->value());
    
    key = core::KeyBoundle::New("bbbb", "a22222", 2, core::Tag::kFlagValue,
                                base::ScopedAllocator{&scope});
    builder.Add(key->tagged_key(), key->value());
    
    key = core::KeyBoundle::New("cccc", "a33333", 3, core::Tag::kFlagValue,
                                base::ScopedAllocator{&scope});
    builder.Add(key->tagged_key(), key->value());
    
    key = core::KeyBoundle::New("dddd", "a44444", 4, core::Tag::kFlagValue,
                                base::ScopedAllocator{&scope});
    builder.Add(key->tagged_key(), key->value());
    
    rs = builder.Finish();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    file->Close();
}

} // namespace table
    
} // namespace mai
