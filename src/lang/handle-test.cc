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
    HandleScope handle_scope(HandleScope::ON_EXIT_SCOPE_INITIALIZER);
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

}

}
