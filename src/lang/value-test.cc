#include "lang/handle.h"
#include "lang/value-inl.h"
#include "lang/metadata.h"
#include "test/isolate-initializer.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class ValueTest : public test::IsolateInitializer {
public:
    // Dummy
};

TEST_F(ValueTest, PrimitiveArray) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    int init[4] = {111, 222, 333, 444};
    
    Handle<Array<int>> handle(Array<int>::NewImmutable(init, arraysize(init)));
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_EQ(4, handle->length());
    ASSERT_EQ(4, handle->capacity());
    
    for (int i = 0; i < arraysize(init); i++) {
        ASSERT_EQ(init[i], handle->At(i));
    }
}

TEST_F(ValueTest, PlusArray) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Handle<Array<int>> handle(Array<int>::NewImmutable(0));
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_EQ(0, handle->length());
    ASSERT_EQ(0, handle->capacity());
    
    for (int i = 0; i < 10; i++) {
        handle = handle->Plus(-1, 1);
    }
}

}

}
