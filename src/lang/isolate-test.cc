#include "lang/isolate-inl.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class IsolateTest : public ::testing::Test {
public:
    // Dummy
};

TEST_F(IsolateTest, Sanity) {
    Isolate *isolate = Isolate::New(Options{});
    ASSERT_NE(nullptr, isolate);
    if (auto err = isolate->Initialize(); err.fail()) {
        isolate->Dispose();
        FAIL() << err.ToString();
    }

    IsolateScope isolate_scope(isolate);
    ASSERT_EQ(isolate, __isolate);
    
    ASSERT_NE(nullptr, isolate->heap());
    ASSERT_NE(nullptr, isolate->metadata_space());
    ASSERT_NE(nullptr, isolate->scheduler());
}

}

}
