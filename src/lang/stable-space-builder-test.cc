#include "lang/stable-space-builder.h"
#include "lang/metadata-space.h"
#include "test/isolate-initializer.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class StableSpaceBuilderTest : public test::IsolateInitializer {
public:
    // Dummy
};


TEST_F(StableSpaceBuilderTest, GlobalSapce) {
    GlobalSpaceBuilder builder;
    ASSERT_EQ(2, builder.length());
    ASSERT_EQ(0, builder.AppendI32(1));
    ASSERT_EQ(64, builder.AppendAny(nullptr));
    ASSERT_EQ(4, builder.AppendI32(2));
    ASSERT_EQ(8, builder.AppendI32(3));
    ASSERT_EQ(12, builder.AppendF32(0.11));
    for (int i = 0; i < 5; i++) {
        builder.AppendI64(8 + i);
    }
    ASSERT_EQ(2, builder.length());
    ASSERT_EQ(2, builder.capacity());

    ASSERT_EQ(56, builder.AppendI64(0));
    ASSERT_EQ(128, builder.AppendF64(1.11));
    ASSERT_EQ(72, builder.AppendAny(nullptr));

    ASSERT_EQ(3, builder.length());
    ASSERT_EQ(4, builder.capacity());
}

TEST_F(StableSpaceBuilderTest, ConstantPool) {
    ConstantPoolBuilder builder;
    
    int loc = builder.FindOrInsertI32(1);
    ASSERT_EQ(0, loc);
    ASSERT_EQ(loc, builder.FindOrInsertI32(1));
    ASSERT_EQ(loc, builder.FindOrInsertU32(1));
    
    loc = builder.FindOrInsertI32(-1);
    ASSERT_EQ(4, loc);
    ASSERT_EQ(loc, builder.FindOrInsertI32(-1));
    ASSERT_EQ(loc, builder.FindOrInsertU32(-1));
    
    loc = builder.FindOrInsertI64(1);
    ASSERT_EQ(8, loc);
    ASSERT_EQ(loc, builder.FindOrInsertI64(1));
    ASSERT_EQ(loc, builder.FindOrInsertU64(1));
    
    ASSERT_EQ(-1, builder.FindString("ok"));
    
    HandleScope handle_scope(HandleScope::INITIALIZER);
    Local<String> s(String::NewUtf8("ok"));
    ASSERT_EQ(32, builder.FindOrInsertString(*s));
    
    ASSERT_EQ(32, builder.FindString("ok"));
}

} // namespace lang

} // namespace mai
