#include "lang/value-inl.h"
#include "lang/metadata.h"
#if defined(MAI_ARCH_X64)
#include "lang/macro-assembler-x64.h"
#endif // defined(MAI_ARCH_X64)
#include "test/isolate-initializer.h"
#include "mai/handle.h"
#include "gtest/gtest.h"
#include <random>

namespace mai {

namespace lang {

class ValueTest : public test::IsolateInitializer {
public:
    // Dummy
};

TEST_F(ValueTest, TypeTraits) {
    ASSERT_STREQ("void", TypeTraits<void>::kName);
    ASSERT_STREQ("i32", TypeTraits<int32_t>::kName);
    ASSERT_STREQ("I32", TypeTraits<Number<int32_t>>::kName);
}

TEST_F(ValueTest, PrimitiveArray) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    int init[4] = {111, 222, 333, 444};
    
    Local<Array<int>> handle(Array<int>::New(init, arraysize(init)));
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_EQ(4, handle->length());
    ASSERT_EQ(4, handle->capacity());
    
    for (int i = 0; i < arraysize(init); i++) {
        ASSERT_EQ(init[i], handle->At(i));
    }
}

TEST_F(ValueTest, PlusPrimitiveArray) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<Array<int>> handle(Array<int>::New(0));
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
    Local<Array<int>> handle(Array<int>::New(init_data, arraysize(init_data)));
    ASSERT_EQ(4, handle->length());
    ASSERT_EQ(4, handle->capacity());
    
    Local<Array<int>> other = handle->Minus(0);
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
    Local<Array<String *>> handle(Array<String *>::New(0));
    
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

static void Dummy1() {}
static void Dummy2(Handle<String>) {}
static void Dummy3(int, Handle<Array<Any*>>) {}
static String *Dummy4(float, int, void *) { return nullptr; }

TEST_F(ValueTest, FunctionTemplate) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    Local<Closure> handle(FunctionTemplate::New(Dummy1));
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_TRUE(handle->is_cxx_function());
    auto code = handle->code();
    ASSERT_NE(nullptr, code);
    ASSERT_EQ(Code::STUB, code->kind());
    ASSERT_EQ("():void", code->prototype()->ToString());

    handle = FunctionTemplate::New(Dummy2);
    ASSERT_TRUE(handle.is_not_empty());
    code = handle->code();
    ASSERT_NE(nullptr, code);
    ASSERT_EQ(Code::STUB, code->kind());
    ASSERT_EQ("(string):void", code->prototype()->ToString());
    
    handle = FunctionTemplate::New(Dummy3);
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_EQ("(i32,array):void", handle->code()->prototype()->ToString());
    
    handle = FunctionTemplate::New(Dummy4);
    ASSERT_TRUE(handle.is_not_empty());
    ASSERT_EQ("(f32,i32,u64):string", handle->code()->prototype()->ToString());
}

TEST_F(ValueTest, AnyIsAs) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Local<Any> handle(Number<int>::ValueOf(0));
    ASSERT_TRUE(handle->Is<Number<int>>());
    
    Local<Number<int>> num = handle->As<Number<int>>();
    ASSERT_EQ(0, num->value());
    
    handle = Local<Any>(String::NewUtf8("Hello"));
    ASSERT_TRUE(handle->Is<String>());
    
    Local<String> str = handle->As<String>();
    ASSERT_STREQ("Hello", str->data());
}

TEST_F(ValueTest, ValueOfNumber) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Local<Number<bool>> b1(Number<bool>::ValueOf(true));
    ASSERT_TRUE(b1->value());
    Local<Number<bool>> b2(Number<bool>::ValueOf(false));
    ASSERT_FALSE(b2->value());
    ASSERT_TRUE(b1 == Number<bool>::ValueOf(true));
    ASSERT_TRUE(b2 == Number<bool>::ValueOf(false));
    
    Local<Number<int8_t>> i8(Number<int8_t>::ValueOf(100));
    ASSERT_EQ(100, i8->value());
    
    Local<Number<int8_t>> i8_2(Number<int8_t>::ValueOf(100));
    ASSERT_EQ(100, i8_2->value());
    ASSERT_TRUE(i8 == i8_2);
}

TEST_F(ValueTest, StringBuilder) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    IncrementalStringBuilder builder;
    builder.AppendString("Hello");
    builder.AppendString(", ");
    builder.AppendString("World");
    builder.AppendFormat("! %d", 101);
    
    Local<String> handle(builder.Build());
    ASSERT_STREQ("Hello, World! 101", handle->data());
}

TEST_F(ValueTest, MapSanity) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Local<Map<int, int>> mm = Map<int, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    ASSERT_EQ(kType_i32, mm->key_type()->id()) << mm->key_type()->name();
    ASSERT_EQ(kType_i32, mm->value_type()->id()) << mm->value_type()->name();
    
    ASSERT_EQ(16, mm->bucket_size());
    ASSERT_EQ(32, mm->capacity());
    
    mm->Set(0, 0);
    mm->Set(1, 100);
    mm->Set(2, 200);
    mm->Set(18, 1800);
    ASSERT_EQ(4, mm->length());
    
    int value;
    ASSERT_TRUE(mm->Get(18, &value));
    ASSERT_EQ(1800, value);
    
    mm->Set(18, 1811);
    ASSERT_EQ(4, mm->length());
    ASSERT_TRUE(mm->Get(18, &value));
    ASSERT_EQ(1811, value);
    
    mm->Erase(18);
    ASSERT_EQ(3, mm->length());
    ASSERT_FALSE(mm->Get(18, &value));
    
    mm->Erase(1);
    ASSERT_EQ(2, mm->length());
    ASSERT_FALSE(mm->Get(1, &value));
    
    mm->Erase(0);
    ASSERT_EQ(1, mm->length());
    ASSERT_FALSE(mm->Get(0, &value));
}

TEST_F(ValueTest, MapReferenceValue) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Local<Map<int, String *>> mm = Map<int, String *>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    ASSERT_EQ(kType_i32, mm->key_type()->id()) << mm->key_type()->name();
    ASSERT_EQ(kType_string, mm->value_type()->id()) << mm->value_type()->name();
    
    Local<String> name(String::NewUtf8("Hello"));
    mm->Set(100, name);
    name = String::NewUtf8("World");
    mm->Set(200, name);
    
    ASSERT_STREQ("Hello", mm->Get(100)->data());
    ASSERT_STREQ("World", mm->Get(200)->data());
    
    mm->Erase(100);
    ASSERT_EQ(1, mm->length());
    ASSERT_TRUE(mm->Get(100).is_empty());
    
    mm->Erase(200);
    ASSERT_EQ(0, mm->length());
    ASSERT_TRUE(mm->Get(200).is_empty());
}

TEST_F(ValueTest, MapReferenceKey) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Local<Map<String *, int>> mm = Map<String *, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    Local<String> name(String::NewUtf8("1st"));
    mm->Set(name, 1);
    name = String::NewUtf8("2nd");
    mm->Set(name, 2);
    name = String::NewUtf8("3rd");
    mm->Set(name, 3);
    
    int value;
    ASSERT_TRUE(mm->Get(String::NewUtf8("1st"), &value));
    ASSERT_EQ(1, value);
    ASSERT_TRUE(mm->Get(String::NewUtf8("2nd"), &value));
    ASSERT_EQ(2, value);
    ASSERT_TRUE(mm->Get(String::NewUtf8("3rd"), &value));
    ASSERT_EQ(3, value);
    
    mm->Erase(String::NewUtf8("2nd"));
    ASSERT_EQ(2, mm->length());
    ASSERT_FALSE(mm->Get(String::NewUtf8("2nd"), &value));
}

TEST_F(ValueTest, MapPutRehash) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    Local<Map<int, int>> mm = Map<int, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    int rehash_count = 0;
    for (int i = 0; i < 128; i++) {
        Map<int, int> *old = *mm;
        mm->Put(i, i * 100, &mm);
        if (*mm != old) {
            rehash_count++;
        }
        ASSERT_TRUE(mm.is_value_not_null());
    }
    
    ASSERT_EQ(2, rehash_count);
    ASSERT_EQ(128, mm->length());
    
    for (int i = 0; i < 128; i++) {
        int value = 0;
        ASSERT_TRUE(mm->Get(i, &value));
        ASSERT_EQ(i * 100, value);
    }
}

TEST_F(ValueTest, MapRemoveRehash) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Local<Map<int, int>> mm = Map<int, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    int rehash_count = 0;
    for (int i = 0; i < 128; i++) {
        Map<int, int> *old = *mm;
        mm->Put(i, i * 100, &mm);
        if (*mm != old) {
            rehash_count++;
        }
        ASSERT_TRUE(mm.is_value_not_null());
    }
    
    ASSERT_EQ(2, rehash_count);
    ASSERT_EQ(128, mm->length());
    
    rehash_count = 0;
    for (int i = 0; i < 128; i++) {
        Map<int, int> *old = *mm;
        mm->Remove(i, &mm);
        if (*mm != old) {
            rehash_count++;
        }
        ASSERT_TRUE(mm.is_value_not_null());
    }
    
    ASSERT_EQ(3, rehash_count);
    ASSERT_EQ(0, mm->length());
    
    mm->Put(996, 700, &mm);
    ASSERT_EQ(1, mm->length());
    int value = 0;
    ASSERT_TRUE(mm->Get(996, &value));
    ASSERT_EQ(700, value);
}

TEST_F(ValueTest, MapFuzzyRehash) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Local<Map<int, int>> mm = Map<int, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    std::vector<int> nums;
    for (int i = 0; i < 10000; i++) {
        nums.push_back(i);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(nums.begin(), nums.end(), g);
    
    for (auto i : nums) { mm->Put(i, -i, &mm); }
    ASSERT_EQ(10000, mm->length());
    ASSERT_EQ(8192, mm->bucket_size());
    
    for (int i = 0; i < 5000; i++) {
        mm->Remove(i, &mm);
    }
    ASSERT_EQ(5000, mm->length());
    for (int i = 0; i < 5000; i++) {
        mm->Put(i, -i, &mm);
    }
    ASSERT_EQ(10000, mm->length());
    
    for (int i = 0; i < 10000; i++) {
        int value = 0;
        ASSERT_TRUE(mm->Get(i, &value));
        ASSERT_EQ(-i, value);
    }
}

TEST_F(ValueTest, MapBaseIterator) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Local<Map<int, int>> mm = Map<int, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    std::set<int> nums;
    for (int i = 0; i < 10000; i++) {
        nums.insert(i);
        mm->Put(i, -i, &mm);
    }
    
    do {
        ImplementMap<int>::Iterator iter(*mm);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            nums.erase(iter->key);
        }
        ASSERT_TRUE(nums.empty());
    } while (0);
    
    do {
        int count = 0;
        Map<int, int>::Iterator iter(mm);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            ASSERT_EQ(iter.key(), -iter.value());
            count++;
        }
        ASSERT_EQ(10000, count);
    } while (0);
}

TEST_F(ValueTest, MapReferenceKeyIterator) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Local<Map<String *, int>> mm = Map<String *, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    std::set<std::string> names {
        "1st",
        "2nd",
        "3rd",
        "4th",
        "aaa",
        "bbb",
        "ccc",
    };
    for (auto name : names) {
        mm->Put(String::NewUtf8(name.data(), name.size()), 110, &mm);
    }
    Map<String *, int>::Iterator iter(mm);
    for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
        ASSERT_EQ(110, iter.value());
        names.erase(iter.key()->data());
    }
    ASSERT_TRUE(names.empty());
}

TEST_F(ValueTest, MapPlusAndMinus) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);

    Local<Map<int, int>> mm = Map<int, int>::New();
    ASSERT_TRUE(mm.is_not_empty());
    ASSERT_TRUE(mm.is_value_not_null());
    
    mm = mm->Plus(1, 100);
    ASSERT_EQ(1, mm->length());
    int value;
    ASSERT_TRUE(mm->Get(1, &value));
    ASSERT_EQ(100, value);

    mm = mm->Minus(1);
    ASSERT_EQ(0, mm->length());
    
    for (int i = 0; i < 1000; i++) {
        mm = mm->Plus(i, -i);
    }
    ASSERT_EQ(1000, mm->length());
    for (int i = 0; i < 1000; i++) {
        int value;
        ASSERT_TRUE(mm->Get(i, &value));
        ASSERT_EQ(i, -value);
    }
}

TEST_F(ValueTest, CodeExecutable) {
    HandleScope handle_scpoe(HandleScope::INITIALIZER);
    
    MacroAssembler masm;
    Generate_SanityTestStub(&masm);
    Local<Kode> code = Kode::New(Code::STUB, 0, masm.buf());
    CallStub<int(int, int), Kode> call_stub(*code);
    ASSERT_EQ(3, call_stub.entry()(1, 2));
    
    EXPECT_EQ(Code::STUB, code->kind());
    EXPECT_EQ(0, code->optimization_level());
    EXPECT_EQ(nullptr, code->source_line_info());
}

}

}
