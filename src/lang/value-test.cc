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

TEST_F(ValueTest, PlusPrimitiveArray) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Handle<Array<int>> handle(Array<int>::NewImmutable(0));
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_EQ(0, handle->length());
    ASSERT_EQ(0, handle->capacity());
    
    for (int i = 0; i < 10; i++) {
        handle = handle->Plus(-1, i);
    }
    ASSERT_EQ(10, handle->length());
    ASSERT_EQ(10, handle->capacity());
    
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(i, handle->At(i));
    }

    handle = handle->Plus(0, 100);
    EXPECT_EQ(100, handle->At(0));
}

TEST_F(ValueTest, MinusPrimitiveArray) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    int init_data[4] = {0, 1, 2, 3};
    Handle<Array<int>> handle(Array<int>::NewImmutable(init_data, arraysize(init_data)));
    ASSERT_EQ(4, handle->length());
    ASSERT_EQ(4, handle->capacity());
    
    Handle<Array<int>> other = handle->Minus(0);
    ASSERT_TRUE(other != handle);
    ASSERT_EQ(3, other->length());
    ASSERT_EQ(3, other->capacity());
    
    ASSERT_EQ(1, other->At(0));
    ASSERT_EQ(2, other->At(1));
    ASSERT_EQ(3, other->At(2));
    
    handle = other->Minus(1);
    ASSERT_EQ(2, handle->length());
    ASSERT_EQ(2, handle->capacity());
    
    ASSERT_EQ(1, handle->At(0));
    ASSERT_EQ(3, handle->At(1));
}

TEST_F(ValueTest, PlusReferenceArray) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Handle<Array<String *>> handle(Array<String *>::NewImmutable(0));
    
    handle = handle->Plus(-1, String::NewUtf8("1st"));
    ASSERT_EQ(1, handle->length());
    ASSERT_EQ(1, handle->capacity());
    
    handle = handle->Plus(-1, String::NewUtf8("2nd"));
    ASSERT_EQ(2, handle->length());
    ASSERT_EQ(2, handle->capacity());
    
    handle = handle->Plus(-1, String::NewUtf8("3rd"));
    ASSERT_EQ(3, handle->length());
    ASSERT_EQ(3, handle->capacity());
    
    ASSERT_STREQ("1st", handle->At(0)->data());
    ASSERT_STREQ("2nd", handle->At(1)->data());
    ASSERT_STREQ("3rd", handle->At(2)->data());
    
    handle = handle->Plus(1, String::NewUtf8("second"));
    ASSERT_STREQ("second", handle->At(1)->data());
}

}

}