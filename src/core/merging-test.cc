#include "core/merging.h"
#include "core/unordered-memory-table.h"
#include "core/internal-key-comparator.h"
#include "core/key-boundle.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "gtest/gtest.h"

namespace mai {

namespace core {

class MergingTest : public ::testing::Test {
public:
    MergingTest()
        : ikcmp_(new core::InternalKeyComparator(Comparator::Bytewise())) {}
    
    void SetUp() override {}
    void TearDown() override {}
    
    
    Env *env_ = Env::Default();
    std::unique_ptr<InternalKeyComparator> ikcmp_;
};
    
TEST_F(MergingTest, UnorderedMerging) {
    using ::mai::core::Tag;
    using ::mai::core::KeyBoundle;
    
    auto t1 = base::MakeRef(new UnorderedMemoryTable(ikcmp_.get(), 13));
    t1->Put("k1", "v1", 1, Tag::kFlagValue);
    t1->Put("k1", "v2", 2, Tag::kFlagValue);
    t1->Put("k1", "v3", 3, Tag::kFlagValue);
    
    auto t2 = base::MakeRef(new UnorderedMemoryTable(ikcmp_.get(), 13));
    t2->Put("k1", "v4", 4, Tag::kFlagValue);
    t2->Put("k2", "v5", 5, Tag::kFlagValue);
    t2->Put("k3", "v6", 6, Tag::kFlagValue);
    
    Iterator *children[2] = {t1->NewIterator(), t2->NewIterator()};
    std::unique_ptr<Iterator>
    merger(Merging::NewUnorderedMergingIterator(ikcmp_.get(), 13, children,
                                                arraysize(children)));

    int n = 0;
    for (merger->SeekToFirst(); merger->Valid(); merger->Next()) {
        std::string key(KeyBoundle::ExtractUserKey(merger->key()));
        std::string value(merger->value());
        ASSERT_EQ('k', key[0]);
        ASSERT_EQ('v', value[0]);
        ++n;
    }
    ASSERT_EQ(6, n);
    
    merger->SeekToFirst();
    ASSERT_TRUE(merger->Valid());
    ASSERT_EQ("v6", merger->value());
    
    merger->Next();
    ASSERT_TRUE(merger->Valid());
    ASSERT_EQ("v5", merger->value());
    
    for (auto iter : children) {
        delete iter;
    }
}
    
TEST_F(MergingTest, UnorderedToMergingIterator) {
    using ::mai::core::Tag;
    using ::mai::core::KeyBoundle;
    
    auto t1 = base::MakeRef(new UnorderedMemoryTable(ikcmp_.get(), 13));
    t1->Put("k1", "v1", 1, Tag::kFlagValue);
    t1->Put("k1", "v2", 2, Tag::kFlagValue);
    t1->Put("k1", "v3", 3, Tag::kFlagValue);
    
    auto t2 = base::MakeRef(new UnorderedMemoryTable(ikcmp_.get(), 13));
    t2->Put("k1", "v4", 4, Tag::kFlagValue);
    t2->Put("k2", "v5", 5, Tag::kFlagValue);
    t2->Put("k3", "v6", 6, Tag::kFlagValue);
    
    Iterator *children[2] = {t1->NewIterator(), t2->NewIterator()};
    
    std::unique_ptr<Iterator>
    merger(Merging::NewMergingIterator(ikcmp_.get(), children,
                                       arraysize(children)));

    int n = 0;
    for (merger->SeekToFirst(); merger->Valid(); merger->Next()) {
        std::string key(KeyBoundle::ExtractUserKey(merger->key()));
        std::string value(merger->value());
        ASSERT_EQ('k', key[0]);
        ASSERT_EQ('v', value[0]);
        ++n;
    }
    ASSERT_EQ(6, n);
    
    merger->Seek(KeyBoundle::MakeKey("k1", 3, Tag::kFlagValueForSeek));
    ASSERT_TRUE(merger->Valid());
    ASSERT_EQ("v3", merger->value());
    
    merger->Seek(KeyBoundle::MakeKey("k1", 5, Tag::kFlagValueForSeek));
    ASSERT_TRUE(merger->Valid());
    ASSERT_EQ("v4", merger->value());
    
    merger->Seek(KeyBoundle::MakeKey("k3", 1, Tag::kFlagValueForSeek));
    ASSERT_FALSE(merger->Valid());
    
    merger->Seek(KeyBoundle::MakeKey("k3", 6, Tag::kFlagValueForSeek));
    ASSERT_TRUE(merger->Valid());
    ASSERT_EQ("v6", merger->value());
}

} // namespace core

} // namespace mai
