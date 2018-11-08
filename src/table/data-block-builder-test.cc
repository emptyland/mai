#include "table/data-block-builder.h"
#include "base/slice.h"
#include "gtest/gtest.h"

namespace mai {

namespace table {
    
TEST(DataBlockBuilderTest, Sanity) {
    DataBlockBuilder bb(3);
    
    bb.Add("aaaa", "v1");
    bb.Add("aaab", "v2");
    bb.Add("aaac", "v3");
    
    std::string rv(bb.Finish());
    const char k[] = "\0\x4" "aaaa\x2v1\x3\x1" "b\x2v2\x3\x1" "c\x2v3\0\0\0\0\x1\0\0\0";
    std::string acu(k, arraysize(k) - 1);
    ASSERT_EQ(acu, rv);
}
    
TEST(DataBlockBuilderTest, Restart) {
    DataBlockBuilder bb(3);
    
    bb.Add("aaaa", "v1");
    bb.Add("aaab", "v2");
    bb.Add("aaac", "v3");
    bb.Add("aaad", "v4");
    
    std::string rv(bb.Finish());
    const char k[] = "\0\x4" "aaaa\x2v1\x3\x1" "b\x2v2\x3\x1" "c\x2v3\0\x4" "aaad\x2v4\0\0\0\0\x15\0\0\0\x2\0\0\0";
    std::string acu(k, arraysize(k) - 1);
    ASSERT_EQ(acu, rv);
}
    
} // namespace table

} // namespace mai
