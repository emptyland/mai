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

TEST_F(StableSpaceBuilderTest, StackSpaceAllocation) {
    StackSpaceAllocator stack;
    
    ASSERT_EQ(72, stack.Reserve(1));
    ASSERT_EQ(76, stack.Reserve(1));
    
    ASSERT_EQ(1, stack.max_spans());
    
    ASSERT_EQ(92, stack.ReserveRef());
    ASSERT_EQ(2, stack.max_spans());
    ASSERT_EQ(80, stack.Reserve(4));
    ASSERT_EQ(84, stack.Reserve(4));
    
    ASSERT_EQ(100, stack.ReserveRef());
    
    ASSERT_EQ(104, stack.Reserve(4));
    ASSERT_EQ(3, stack.max_spans());
}

TEST_F(StableSpaceBuilderTest, StackSpaceFallback) {
    StackSpaceAllocator stack;
    
    ASSERT_EQ(72, stack.Reserve(1));
    ASSERT_EQ(76, stack.Reserve(1));
    ASSERT_EQ(8, stack.level().p);
    
    stack.Fallback(76, 1);
    ASSERT_EQ(4, stack.level().p);
    
    stack.Fallback(72, 1);
    ASSERT_EQ(0, stack.level().p);
    
    ASSERT_EQ(92, stack.ReserveRef());
    ASSERT_EQ(100, stack.ReserveRef());
    ASSERT_EQ(32, stack.level().r);
    ASSERT_EQ(108, stack.ReserveRef());
    ASSERT_EQ(40, stack.level().r);
    stack.FallbackRef(108);
    ASSERT_EQ(32, stack.level().r);
    
    ASSERT_EQ(76, stack.Reserve(8));
    ASSERT_EQ(8, stack.level().p);
    stack.Fallback(76, 8);
    ASSERT_EQ(0, stack.level().p);
}

TEST_F(StableSpaceBuilderTest, StackSpaceFallbackRef) {
    StackSpaceAllocator stack;
    
    ASSERT_EQ(72, stack.Reserve(1));
    ASSERT_EQ(76, stack.Reserve(1));
    ASSERT_EQ(8, stack.level().p);
    
    ASSERT_EQ(0, stack.level().r);
    ASSERT_EQ(92, stack.ReserveRef());
    ASSERT_EQ(24, stack.level().r);
    stack.FallbackRef(92);
    ASSERT_EQ(16, stack.level().r);
    
    ASSERT_EQ(92, stack.ReserveRef());
    ASSERT_EQ(24, stack.level().r);
}

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
