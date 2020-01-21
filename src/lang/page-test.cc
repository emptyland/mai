#include "lang/page.h"
#include "mai/allocator.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class PageTest : public ::testing::Test {
public:
    using Bitmap = AllocationBitmap<32>;
    
    // Dummy
    PageTest() {
        ::memset(bitmap_, 0, sizeof(bitmap_));
    }

    uint32_t bitmap_[32];
};

TEST_F(PageTest, Sanity) {
    ASSERT_EQ(1024*1024, kPageSize);
    
    printf("%d %d\n", (int)Page::kChunkSize, (int)Page::kBitmapSize);
}

TEST_F(PageTest, BitmapSanity) {
    auto bmp = Bitmap::FromArray(bitmap_);
    bmp->MarkAllocated(0, 16);
    ASSERT_TRUE(bmp->HasAllocated(0));
    ASSERT_EQ(16, bmp->AllocatedSize(0));
    
    bmp->ClearAllocated(0);
    ASSERT_FALSE(bmp->HasAllocated(0));
    ASSERT_EQ(0, bmp->AllocatedSize(0));
}

TEST_F(PageTest, BitmapBig) {
    auto bmp = Bitmap::FromArray(bitmap_);
    bmp->MarkAllocated(16, 512);
    ASSERT_FALSE(bmp->HasAllocated(0));
    ASSERT_TRUE(bmp->HasAllocated(16));
    ASSERT_EQ(512, bmp->AllocatedSize(16));
}

} // namespace lang

} // namespace mai
