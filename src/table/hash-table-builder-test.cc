#include "table/hash-table-builder.h"
#include "table/hash-table-reader.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/hash.h"
#include "base/allocators.h"
#include "mai/env.h"
#include "mai/options.h"
#include "mai/iterator.h"
#include "mai/comparator.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace table {

TEST(HashTableBuilderTest, Sanity) {
    auto env = Env::Default();
    
    std::unique_ptr<WritableFile> file;
    auto rs = env->NewWritableFile("tests/02-hash-table-file.tmp", &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    rs = file->Truncate(0);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    base::ScopedMemory scope;
    HashTableBuilder builder(file.get(), 103, &base::Hash::Js, 512);
    auto key = core::KeyBoundle::New("aaaa", "a11111", 1, core::Tag::kFlagValue,
                                     base::ScopedAllocator{&scope});
    builder.Add(key->key(), key->value());
    
    key = core::KeyBoundle::New("bbbb", "a22222", 2, core::Tag::kFlagValue,
                                base::ScopedAllocator{&scope});
    builder.Add(key->key(), key->value());
    
    key = core::KeyBoundle::New("cccc", "a33333", 3, core::Tag::kFlagValue,
                                base::ScopedAllocator{&scope});
    builder.Add(key->key(), key->value());
    
    key = core::KeyBoundle::New("dddd", "a44444", 4, core::Tag::kFlagValue,
                                base::ScopedAllocator{&scope});
    builder.Add(key->key(), key->value());
    
    rs = builder.Finish();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    file->Close();
}
    
TEST(HashTableBuilderTest, Reading) {
    auto env = Env::Default();
    std::unique_ptr<core::InternalKeyComparator> ikcmp(new core::InternalKeyComparator(Comparator::Bytewise()));
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env->NewRandomAccessFile("tests/02-hash-table-file.tmp", &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size;
    rs = file->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    HashTableReader reader(file.get(), file_size, &base::Hash::Js);
    rs = reader.Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    base::ScopedMemory scope;
    auto key = core::KeyBoundle::New("aaaa", 1, base::ScopedAllocator{&scope});
    std::string_view value;
    std::string scratch;
    core::Tag tag;
    reader.TableReader::Prepare(key->key());
    rs = reader.Get(ReadOptions{}, ikcmp.get(), key->key(),
                    &tag, &value, &scratch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("a11111", value);
    ASSERT_EQ(1, tag.version());
    ASSERT_EQ(core::Tag::kFlagValue, tag.flags());
    
    file->Close();
}
    
TEST(HashTableBuilderTest, Iterating) {
    auto env = Env::Default();
    std::unique_ptr<core::InternalKeyComparator> ikcmp(new core::InternalKeyComparator(Comparator::Bytewise()));
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env->NewRandomAccessFile("tests/02-hash-table-file.tmp", &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size;
    rs = file->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    HashTableReader reader(file.get(), file_size, &base::Hash::Js);
    rs = reader.Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::unique_ptr<Iterator>
    iter(reader.NewIterator(ReadOptions{}, ikcmp.get()));
    
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        printf("%s:%s\n", iter->key().data(), iter->value().data());
    }
}

} // namespace table
    
} // namespace mai
