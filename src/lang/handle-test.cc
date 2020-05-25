#include "lang/handle.h"
#include "lang/value-inl.h"
#include "lang/metadata.h"
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
    Local<String> handle = String::NewUtf8("Hello, 世界!");
    
    ASSERT_EQ(1, handle_scope.GetNumberOfHanldes());
    ASSERT_EQ(&handle_scope, HandleScope::Current());

    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_TRUE(handle.is_value_not_null());
    ASSERT_STREQ("Hello, 世界!", handle->data());
    ASSERT_EQ(14, handle->length());
    ASSERT_EQ(15, handle->capacity());
    
    ASSERT_NE(nullptr, handle->clazz());
    ASSERT_STREQ("string", handle->clazz()->name());

    Local<Any> other(handle);
    ASSERT_EQ(2, handle_scope.GetNumberOfHanldes());
}

template<class T, bool R = ElementTraits<T>::kIsReferenceType>
class Dummy {
public:
    T To() { return T(0); }
}; // class Dummy

template<class T>
class Dummy<T, true> {
public:
    Local<T> To() { return Local<T>::Empty(); }
}; // class Dummy

TEST_F(HandleTest, TypeTraitsDummy) {
    Dummy<int> dummy1{};
    ASSERT_EQ(0, dummy1.To());
    
    Dummy<String> dummy2{};
    Local<String> handle = dummy2.To();
    ASSERT_TRUE(handle.is_empty());
}

TEST_F(HandleTest, ArrayHandle) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    int data[4] = {1, 2, 3, 4};
    Local<Array<int32_t>> handle = Array<int32_t>::New(data, arraysize(data));
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_TRUE(handle.is_value_not_null());
    ASSERT_EQ(4, handle->length());
    ASSERT_EQ(4, handle->capacity());
    ASSERT_EQ(1, handle->At(0));
    ASSERT_EQ(2, handle->At(1));
    ASSERT_EQ(3, handle->At(2));
    ASSERT_EQ(4, handle->At(3));
}

TEST_F(HandleTest, ReferenceArrayHandle) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<String> init[4] = {
        String::NewUtf8("1st"),
        String::NewUtf8("2nd"),
        String::NewUtf8("3rd"),
        String::NewUtf8("4th"),
    };
    Local<Array<String *>> handle = Array<String *>::New(&init[0], arraysize(init));
    ASSERT_EQ(4, handle->length());
    ASSERT_EQ(4, handle->capacity());
    ASSERT_EQ(STATE->builtin_type(kType_array), handle->clazz());

    Local<String> elem = handle->At(0);
    ASSERT_STREQ("1st", elem->data());
    ASSERT_STREQ("2nd", handle->At(1)->data());
    ASSERT_STREQ("3rd", handle->At(2)->data());
    ASSERT_STREQ("4th", handle->At(3)->data());
}

TEST_F(HandleTest, GlobalHandle) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<String> local = String::NewUtf8("123");
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
