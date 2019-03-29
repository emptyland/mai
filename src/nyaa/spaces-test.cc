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
//    std::unique_ptr<NewSpace> ns(new NewSpace);
//    auto rs = ns->Init(N_->major_area_initial_size(), isolate_);
//    ASSERT_TRUE(rs.ok()) << rs.ToString();
//    
//    ASSERT_EQ(N_->major_area_initial_size()/2, ns->Available());
//    
//    auto available = ns->Available();
//    auto chunk = ns->AllocateRaw(sizeof(NyFloat64));
//    auto f64 = new (chunk) NyFloat64(100);
//    ASSERT_EQ(f64->value(), 100);
//    ASSERT_EQ(available - ns->Available(), f64->PlacedSize());
//    
//    available = ns->Available();
//    chunk = ns->AllocateRaw(NyString::RequiredSize(4));
//    auto str = new (chunk) NyString("abcd", 4, core_);
//    ASSERT_STREQ("abcd", str->bytes());
//    ASSERT_EQ(available - ns->Available(), str->PlacedSize());
//    
//    int n = 0;
//    while ((chunk = ns->AllocateRaw(128)) != nullptr) {
//        ++n;
//    }
//    ASSERT_EQ(40959, n);
}
    
} // namespace nyaa

} // namespace mai
