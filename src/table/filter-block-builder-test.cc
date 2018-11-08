#include "table/filter-block-builder.h"
#include "base/hash.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace table {
    
class FilterBlockBuilderTest : public ::testing::Test {
public:
    FilterBlockBuilderTest() {
        hashs_[0] = &base::Hash::Js;
        hashs_[1] = &base::Hash::Rs;
        hashs_[2] = &base::Hash::Bkdr;
        hashs_[3] = &base::Hash::Sdbm;
        hashs_[4] = &base::Hash::Elf;
    }

    base::hash_func_t hashs_[5];
};
    
TEST_F(FilterBlockBuilderTest, Sanity) {
    
    FilterBlockBuilder fbb(512, hashs_, arraysize(hashs_));
    ASSERT_EQ(512 * 8, fbb.n_bits());
    
    fbb.AddKey("aaaaa");
    fbb.AddKey("bbbbb");
    fbb.AddKey("ccccc");
    
    ASSERT_TRUE(fbb.MayExist("aaaaa"));
    ASSERT_TRUE(fbb.MayExist("bbbbb"));
    ASSERT_TRUE(fbb.MayExist("ccccc"));
    ASSERT_TRUE(fbb.EnsureNotExist("d"));
}
    
} // namespace table
    
} // namespace mai
