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

    Env *env_ = Env::Default();
    Allocator *lla_ = env_->GetLowLevelAllocator();
    uint32_t bitmap_[32];
};

TEST_F(PageTest, Sanity) {
    ASSERT_EQ(1024*1024, kPageSize);
    ASSERT_EQ(1032373, Page::kChunkSize);
    ASSERT_EQ(4032, Page::kBitmapSize);
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

TEST_F(PageTest, BitmapContinuous) {
    auto bmp = Bitmap::FromArray(bitmap_);
    bmp->MarkAllocated(0, 16);
    bmp->MarkAllocated(16, 16);
    bmp->MarkAllocated(32, 16);
    bmp->MarkAllocated(48, 16);
    
    ASSERT_EQ(16, bmp->AllocatedSize(0));
    ASSERT_EQ(16, bmp->AllocatedSize(16));
    ASSERT_EQ(16, bmp->AllocatedSize(32));
    ASSERT_EQ(16, bmp->AllocatedSize(48));
    
    bmp->ClearAllocated(32);
    ASSERT_EQ(0, bmp->AllocatedSize(32));
}

TEST_F(PageTest, BitmapBig) {
    auto bmp = Bitmap::FromArray(bitmap_);
    bmp->MarkAllocated(16, 512);
    ASSERT_FALSE(bmp->HasAllocated(0));
    ASSERT_TRUE(bmp->HasAllocated(16));
    ASSERT_EQ(512, bmp->AllocatedSize(16));
}

TEST_F(PageTest, AllocateNormalPage) {
    auto page = Page::New(kOldSpace, Allocator::kRd|Allocator::kWr, lla_);
    ASSERT_NE(nullptr, page);
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(page) % kPageSize);
    
    EXPECT_EQ(kOldSpace, page->owner_space());
    EXPECT_EQ(1032376, page->available());
    
    auto bmp = page->bitmap();
    ASSERT_NE(nullptr, bmp);
    
    page->Dispose(lla_);
}

TEST_F(PageTest, AllocateLargePage) {
    auto page = LargePage::New(kLargeSpace, Allocator::kRd|Allocator::kWr, 10 * 1024 * 1024, lla_);
    ASSERT_NE(nullptr, page);
    ASSERT_EQ(0, reinterpret_cast<uintptr_t>(page) % kPageSize);
    
    EXPECT_EQ(kLargeSpace, page->owner_space());
    EXPECT_EQ(10485720, page->available());
    EXPECT_EQ(10 * 1024 * 1024, page->size());
    
    page->Dispose(lla_);
}

} // namespace lang

} // namespace mai
