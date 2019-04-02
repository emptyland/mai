#include "nyaa/spaces.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {

namespace nyaa {
    
class NyaaSpacesTest : public test::NyaaTest {
public:
    NyaaSpacesTest()
        : scope_(Nyaa::Options{}, isolate_)
        , N_(&scope_)
        , core_(scope_.core()) {
    }
    
    void SetUp() override {
        NyaaTest::SetUp();
    }
    
    void TearDown() override {
        NyaaTest::TearDown();
    }
    
    Nyaa scope_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
};
    
TEST_F(NyaaSpacesTest, NewSpace) {
    std::unique_ptr<NewSpace> ns(new NewSpace);
    auto rs = ns->Init(N_->major_area_initial_size(), isolate_);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    ASSERT_GE(N_->major_area_initial_size()/2, ns->Available());
    
    auto available = ns->Available();
    auto chunk = ns->AllocateRaw(sizeof(NyFloat64));
    auto f64 = new (chunk) NyFloat64(100);
    ASSERT_EQ(f64->value(), 100);
    ASSERT_EQ(available - ns->Available(), f64->PlacedSize());
    
    available = ns->Available();
    chunk = ns->AllocateRaw(NyString::RequiredSize(4));
    auto str = new (chunk) NyString("abcd", 4, core_);
    ASSERT_STREQ("abcd", str->bytes());
    ASSERT_EQ(available - ns->Available(), str->PlacedSize());
    
    int n = 0;
    while ((chunk = ns->AllocateRaw(128)) != nullptr) {
        ++n;
    }
    ASSERT_EQ(40959, n);
}
    
TEST_F(NyaaSpacesTest, OldSpace) {
    std::unique_ptr<OldSpace> os(new OldSpace(kOldSpace, false, isolate_));
    auto rs = os->Init(N_->minor_area_initial_size(), N_->minor_area_max_size());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_GE(N_->minor_area_initial_size(), os->Available());
    
    auto available = os->Available();
    auto chunk = os->AllocateRaw(sizeof(NyFloat64));
    auto f64 = new (chunk) NyFloat64(100);
    f64->SetColor(kColorWhite);
    ASSERT_EQ(f64->value(), 100);
    ASSERT_EQ(available - os->Available(), f64->PlacedSize());
    ASSERT_EQ(kColorWhite, f64->GetColor());
    ASSERT_EQ(kColorWhite, os->GetAddressColor(reinterpret_cast<Address>(f64)));
}
    
TEST_F(NyaaSpacesTest, OldSpaceInit) {
    std::unique_ptr<OldSpace> os(new OldSpace(kOldSpace, false, isolate_));
    auto rs = os->Init(N_->minor_area_initial_size(), N_->minor_area_max_size());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_GE(N_->minor_area_initial_size(), os->Available());
    
    auto region = os->cache()->region();
    ASSERT_EQ(nullptr, region->tiny);
    ASSERT_EQ(nullptr, region->small);
    ASSERT_EQ(nullptr, region->medium);
    ASSERT_EQ(nullptr, region->normal);
    ASSERT_NE(nullptr, region->big);
    
    auto chunk = region->big;
    ASSERT_EQ(nullptr, chunk->next);
    ASSERT_EQ(1048520, chunk->size);
    ASSERT_EQ(os->cache_->available(), chunk->size);
}
    
TEST_F(NyaaSpacesTest, OldSpaceAlloc) {
    std::unique_ptr<OldSpace> os(new OldSpace(kOldSpace, false, isolate_));
    auto rs = os->Init(N_->minor_area_initial_size(), N_->minor_area_max_size());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto blk = os->AllocateRaw(sizeof(NyFloat64));
    ASSERT_EQ(kColorWhite, os->GetAddressColor(blk));
    
    auto chunk = os->cache()->region()->big;
    ASSERT_EQ(nullptr, chunk->next);
    ASSERT_EQ(1048504, chunk->size);
    ASSERT_EQ(os->cache()->available(), chunk->size);
    
    blk = os->AllocateRaw(sizeof(NyFloat64));
    ASSERT_EQ(kColorWhite, os->GetAddressColor(blk));
    
    chunk = os->cache()->region()->big;
    ASSERT_EQ(nullptr, chunk->next);
    ASSERT_EQ(1048488, chunk->size);
    ASSERT_EQ(os->cache()->available(), chunk->size);
}

TEST_F(NyaaSpacesTest, OldSpaceFreeMerge) {
    std::unique_ptr<OldSpace> os(new OldSpace(kOldSpace, false, isolate_));
    auto rs = os->Init(N_->minor_area_initial_size(), N_->minor_area_max_size());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto b1 = os->AllocateRaw(sizeof(NyFloat64));
    auto b2 = os->AllocateRaw(sizeof(NyFloat64));
    
    os->Free(b2, sizeof(NyFloat64));
    os->Free(b1, sizeof(NyFloat64));

    auto chunk = os->cache()->region()->big;
    ASSERT_EQ(nullptr, chunk->next);
    ASSERT_EQ(1048520, chunk->size);
    ASSERT_EQ(os->cache()->available(), chunk->size);
}
    
TEST_F(NyaaSpacesTest, OldSpaceFreeNoMerge) {
    std::unique_ptr<OldSpace> os(new OldSpace(kOldSpace, false, isolate_));
    auto rs = os->Init(N_->minor_area_initial_size(), N_->minor_area_max_size());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto b1 = os->AllocateRaw(sizeof(NyFloat64));
    auto b2 = os->AllocateRaw(sizeof(NyFloat64));
    
    os->Free(b1, sizeof(NyFloat64));
    os->Free(b2, sizeof(NyFloat64));
    
    auto chunk = os->cache()->region()->big;
    ASSERT_EQ(nullptr, chunk->next);
    ASSERT_EQ(1048504, chunk->size);

    chunk = os->cache()->region()->tiny;
    ASSERT_EQ(nullptr, chunk->next);
    ASSERT_EQ(sizeof(NyFloat64), chunk->size);
}

TEST_F(NyaaSpacesTest, LargeSpace) {
    std::unique_ptr<LargeSpace> ls(new LargeSpace(N_->minor_area_max_size(), isolate_));
    ls->Free(ls->AllocateRaw(1));
}
    
} // namespace nyaa

} // namespace mai
