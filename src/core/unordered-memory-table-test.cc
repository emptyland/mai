#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "core/unordered-memory-table.h"
#include "base/hash.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "gtest/gtest.h"

namespace {
    
using ::mai::core::KeyBoundle;
using ::mai::core::InternalKeyComparator;
using ::mai::base::Hash;
    
struct TestComparator {
    const InternalKeyComparator *ikcmp_;
    int operator ()(const KeyBoundle *lhs, const KeyBoundle *rhs) const {
        return ikcmp_->Compare(lhs->key(), rhs->key());
    }
    int operator ()(const KeyBoundle *key) const {
        return Hash::Js(key->user_key().data(),
                        key->user_key().size()) & 0x7fffffff;
    }
};
    
} // namespace

namespace mai {
    
namespace core {
    
class UnorderedMemoryTableTest : public ::testing::Test {
public:
    UnorderedMemoryTableTest()
        : ikcmp_(new core::InternalKeyComparator(Comparator::Bytewise())) {}
    
    void SetUp() override {
        arena_ = new base::Arena(env_->GetLowLevelAllocator());
    }
    
    void TearDown() override {
        delete arena_;
    }
    
    Env *env_ = Env::Default();
    std::unique_ptr<InternalKeyComparator> ikcmp_;
    base::Arena *arena_ = nullptr;
};

TEST_F(UnorderedMemoryTableTest, KeyBoundleMap) {
    HashMap<KeyBoundle *, TestComparator> map(8, TestComparator{ikcmp_.get()}, arena_);
    map.Put(KeyBoundle::New("k1", "v1", 1, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k1", "v2", 2, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k2", "v2", 3, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k3", "v3", 4, Tag::kFlagValue));
    map.Put(KeyBoundle::New("k4", "v4", 5, Tag::kFlagValue));
    ASSERT_EQ(5, map.n_entries());
    
    base::ScopedMemory scope_memory;
    HashMap<KeyBoundle *, TestComparator>::Iterator iter(&map);
    
    auto lookup = KeyBoundle::New("k1", 6, base::ScopedAllocator{&scope_memory});
    iter.Seek(lookup);
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(0, ::strncmp(iter.key()->value().data(),
                           "v2", iter.key()->value().size()));
    
    lookup = KeyBoundle::New("k1", 1, base::ScopedAllocator{&scope_memory});
    iter.Seek(lookup);
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(0, ::strncmp(iter.key()->value().data(),
                           "v1", iter.key()->value().size()));
}
    
TEST_F(UnorderedMemoryTableTest, MemoryTable) {
    auto table = base::MakeRef(new UnorderedMemoryTable(ikcmp_.get(), 13, arena_));
    table->Put("k1", "v1", 1, Tag::kFlagValue);
    table->Put("k1", "v3", 3, Tag::kFlagValue);
    table->Put("k1", "v5", 5, Tag::kFlagValue);
    
    std::string value;
    auto rs = table->Get("k1", 2, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v1", value);
    
    rs = table->Get("k1", 5, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v5", value);
    
    rs = table->Get("k2", 5, nullptr, &value);
    ASSERT_TRUE(rs.IsNotFound());
}
    
TEST_F(UnorderedMemoryTableTest, Iterate) {
    auto table = base::MakeRef(new UnorderedMemoryTable(ikcmp_.get(), 13, arena_));
    table->Put("k1", "v1", 1, Tag::kFlagValue);
    table->Put("k1", "v3", 3, Tag::kFlagValue);
    table->Put("k1", "v5", 5, Tag::kFlagValue);
    ASSERT_EQ(1, table->ref_count());
    
    std::unique_ptr<Iterator> iter(table->NewIterator());
    ASSERT_EQ(2, table->ref_count());
    
    ASSERT_TRUE(iter->error().ok()) << iter->error().ToString();
    ASSERT_FALSE(iter->Valid());
    
    iter->SeekToFirst();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("v5", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("v3", iter->value());
    
    iter->Next();
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ("v1", iter->value());
    
    iter->Next();
    ASSERT_FALSE(iter->Valid());
    
    iter.reset();
    ASSERT_EQ(1, table->ref_count());
}

TEST_F(UnorderedMemoryTableTest, Deletion) {
    auto table = base::MakeRef(new UnorderedMemoryTable(ikcmp_.get(), 13, arena_));
    table->Put("k1", "v1", 1, Tag::kFlagValue);
    table->Put("k1", "v3", 3, Tag::kFlagValue);
    table->Put("k1", "v5", 5, Tag::kFlagValue);
    table->Put("k1", "", 2, Tag::kFlagDeletion);
    
    std::string value;
    auto rs = table->Get("k1", 6, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v5", value);
    
    core::Tag tag;
    rs = table->Get("k1", 2, &tag, &value);
    ASSERT_TRUE(rs.ok());
    ASSERT_EQ(core::Tag::kFlagDeletion, tag.flag());
    
    rs = table->Get("k1", 1, nullptr, &value);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v1", value);
}

} // namespace core
    
} // namespace mai
