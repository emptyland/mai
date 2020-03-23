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
    
    int off1 = BytecodeStackFrame::kOffsetHeaderSize + kStackSizeGranularity;
    ASSERT_EQ(off1, stack.Reserve(1));
    off1 += kStackSizeGranularity;
    ASSERT_EQ(off1, stack.Reserve(1));
    
    ASSERT_EQ(1, stack.max_spans());
    
    int off2 = BytecodeStackFrame::kOffsetHeaderSize + StackSpaceAllocator::kSpanSize + kPointerSize;
    ASSERT_EQ(off2, stack.ReserveRef());
    ASSERT_EQ(2, stack.max_spans());
    off1 += 4;
    ASSERT_EQ(off1, stack.Reserve(4));
    off1 += 4;
    ASSERT_EQ(off1, stack.Reserve(4));
    
    off2 += kPointerSize;
    ASSERT_EQ(off2, stack.ReserveRef());
    
    off1 = off2 + 4;
    ASSERT_EQ(off1, stack.Reserve(4));
    ASSERT_EQ(3, stack.max_spans());
}

TEST_F(StableSpaceBuilderTest, StackSpaceFallback) {
    StackSpaceAllocator stack;
    
    int off1 = BytecodeStackFrame::kOffsetHeaderSize + kStackSizeGranularity;
    ASSERT_EQ(off1, stack.Reserve(1));
    off1 += kStackSizeGranularity;
    ASSERT_EQ(off1, stack.Reserve(1));
    ASSERT_EQ(8, stack.level().p);
    
    stack.Fallback(off1, 1);
    off1 -= kStackSizeGranularity;
    ASSERT_EQ(4, stack.level().p);
    
    stack.Fallback(off1, 1);
    off1 -= kStackSizeGranularity;
    ASSERT_EQ(0, stack.level().p);
    
    int off2 = BytecodeStackFrame::kOffsetHeaderSize + StackSpaceAllocator::kSpanSize + kPointerSize;
    ASSERT_EQ(off2, stack.ReserveRef());
    off2 += kPointerSize;
    ASSERT_EQ(off2, stack.ReserveRef());
    ASSERT_EQ(32, stack.level().r);
    off2 += kPointerSize;
    ASSERT_EQ(off2, stack.ReserveRef());
    ASSERT_EQ(40, stack.level().r);
    stack.FallbackRef(off2);
    off2 -= kPointerSize;
    ASSERT_EQ(32, stack.level().r);
    
    off1 += 8;
    ASSERT_EQ(off1, stack.Reserve(8));
    ASSERT_EQ(8, stack.level().p);
    stack.Fallback(off1, 8);
    ASSERT_EQ(0, stack.level().p);
}

TEST_F(StableSpaceBuilderTest, StackSpaceFallbackRef) {
    StackSpaceAllocator stack;
    
    int off1 = BytecodeStackFrame::kOffsetHeaderSize + kStackSizeGranularity;
    ASSERT_EQ(off1, stack.Reserve(1));
    off1 += kStackSizeGranularity;
    ASSERT_EQ(off1, stack.Reserve(1));
    ASSERT_EQ(8, stack.level().p);
    
    ASSERT_EQ(0, stack.level().r);
    int off2 = BytecodeStackFrame::kOffsetHeaderSize + StackSpaceAllocator::kSpanSize + kPointerSize;
    ASSERT_EQ(off2, stack.ReserveRef());
    ASSERT_EQ(24, stack.level().r);
    stack.FallbackRef(off2);
    ASSERT_EQ(16, stack.level().r);
    
    ASSERT_EQ(off2, stack.ReserveRef());
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
