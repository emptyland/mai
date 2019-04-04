#include "nyaa/string-pool.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/malloc-object-factory.h"
#include "base/slice.h"
#include "test/nyaa-test.h"
#include "mai-lang/nyaa.h"

namespace mai {
    
namespace nyaa {

class NyaaStringPoolTest : public test::NyaaTest {
public:
    NyaaStringPoolTest()
        : scope_(Nyaa::Options{}, isolate_)
        , N_(&scope_)
        , core_(scope_.core())
        , lla_(isolate_->env()->GetLowLevelAllocator())
        , pool_(lla_)
        , factory_(new MallocObjectFactory(core_)) {
    }
    
    Nyaa scope_;
    Nyaa *N_ = nullptr;
    NyaaCore *core_ = nullptr;
    Allocator *lla_ = nullptr;
    StringPool pool_;
    std::unique_ptr<ObjectFactory> factory_;
};
    
TEST_F(NyaaStringPoolTest, Sanity) {
    ASSERT_EQ(nullptr, pool_.GetOrNull("100"));
    ASSERT_EQ(nullptr, pool_.GetOrNull("200"));
    ASSERT_EQ(nullptr, pool_.GetOrNull("300"));
    
    pool_.Add(factory_->NewString("100"));
    pool_.Add(factory_->NewString("200"));
    pool_.Add(factory_->NewString("300"));
    
    ASSERT_EQ(3, pool_.size());
    
    auto s = pool_.GetOrNull("100");
    ASSERT_NE(nullptr, s);
    ASSERT_STREQ("100", s->bytes());
    
    s = pool_.GetOrNull("200");
    ASSERT_NE(nullptr, s);
    ASSERT_STREQ("200", s->bytes());
    
    s = pool_.GetOrNull("300");
    ASSERT_NE(nullptr, s);
    ASSERT_STREQ("300", s->bytes());
}
    
TEST_F(NyaaStringPoolTest, Extend) {
    ASSERT_EQ(StringPool::kInitSlots, pool_.n_slots());
    
    for (int i = 0; i < 1000; ++i) {
        std::string s = base::Sprintf("s.%03d\n", i);
        pool_.Add(factory_->NewString(s.data(), s.length()));
    }
    ASSERT_EQ(1000, pool_.size());
    ASSERT_EQ(1023, pool_.n_slots());
    
    for (int i = 0; i < 1000; ++i) {
        std::string k = base::Sprintf("s.%03d\n", i);
        auto s = pool_.GetOrNull(k.data(), k.length());
        ASSERT_NE(nullptr, s);
        ASSERT_EQ(k, s->bytes());
    }
}
    
TEST_F(NyaaStringPoolTest, Sweep) {
    ASSERT_EQ(StringPool::kInitSlots, pool_.n_slots());
    for (int i = 0; i < 1000; ++i) {
        std::string s = i % 2 ? base::Sprintf("i.%03d\n", i) : base::Sprintf("s.%03d\n", i);
        pool_.Add(factory_->NewString(s.data(), s.length()));
    }
    ASSERT_EQ(1000, pool_.size());
    ASSERT_EQ(1023, pool_.n_slots());
    
    auto n = pool_.Sweep([] (const NyString *s) { return s->bytes()[0] == 'i'; });
    ASSERT_EQ(500, n);
    ASSERT_EQ(500, pool_.size());
    ASSERT_EQ(1023, pool_.n_slots());
    
    for (int i = 0; i < 1000; ++i) {
        std::string k;
        if (i % 2) {
            k = base::Sprintf("i.%03d\n", i);
            ASSERT_EQ(nullptr, pool_.GetOrNull(k.data(), k.length()));
        } else {
            k = base::Sprintf("s.%03d\n", i);
            auto s = pool_.GetOrNull(k.data(), k.length());
            ASSERT_NE(nullptr, s);
            ASSERT_EQ(k, s->bytes());
        }
    }
}

} // namespace nyaa
    
} // namespace mai
