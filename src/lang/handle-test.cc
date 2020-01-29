#include "lang/handle.h"
#include "lang/value-inl.h"
#include "test/isolate-initializer.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class HandleTest : public test::IsolateInitializer {
public:
    // Dummy
};

TEST_F(HandleTest, StringHandle) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Handle<String> handle = String::NewUtf8("Hello, 世界!");
    
    ASSERT_EQ(1, handle_scope.GetNumberOfHanldes());
    ASSERT_EQ(&handle_scope, HandleScope::Current());

    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_TRUE(handle.is_value_not_null());
    ASSERT_STREQ("Hello, 世界!", handle->data());
    ASSERT_EQ(14, handle->length());
    ASSERT_EQ(15, handle->capacity());
    
    ASSERT_NE(nullptr, handle->clazz());
    ASSERT_STREQ("string", handle->clazz()->name());

    Handle<Any> other(handle);
    ASSERT_EQ(2, handle_scope.GetNumberOfHanldes());
}

TEST_F(HandleTest, GlobalHandle) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Handle<String> local = String::NewUtf8("123");
    Persistent<String> handle = Persistent<String>::New(local);
    
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_TRUE(handle.is_value_not_null());
    ASSERT_EQ(*local, *handle);
    ASSERT_TRUE(handle == local);
    ASSERT_EQ(1, GlobalHandles::GetNumberOfHandles());
    
    Persistent<String> other(handle);
    ASSERT_EQ(2, GlobalHandles::GetNumberOfHandles());
    
    handle.Dispose();
    ASSERT_TRUE(handle.is_empty());
    
    other.Dispose();
    ASSERT_EQ(0, GlobalHandles::GetNumberOfHandles());
    
    handle = local;
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_TRUE(handle.is_value_not_null());
    ASSERT_EQ(1, GlobalHandles::GetNumberOfHandles());
}

}

}
