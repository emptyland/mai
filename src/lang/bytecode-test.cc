#include "lang/bytecode.h"
#include "lang/bytecode-array-builder.h"
#include "test/isolate-initializer.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class BytecodeTest : public test::IsolateInitializer {
public:
    // Dummy
    void SetUp() override {
        test::IsolateInitializer::SetUp();
        arean_ = new base::StandaloneArena(isolate_->env()->GetLowLevelAllocator());
    }
    
    void TearDown() override {
        delete arean_;
        test::IsolateInitializer::TearDown();
    }

    base::Arena *arean_ = nullptr;
};

TEST_F(BytecodeTest, Sanity) {
    auto node = Bytecodes<kLdar64>::New(arean_, 16);
    ASSERT_NE(nullptr, node);
    ASSERT_EQ(BytecodeType::A, node->kind());
    ASSERT_EQ(16, node->param(0));
    
    node = Bytecodes<kLdaPropertyPtr>::New(arean_, 16, 32);
    ASSERT_NE(nullptr, node);
    ASSERT_EQ(BytecodeType::AB, node->kind());
    ASSERT_EQ(16, node->param(0));
    ASSERT_EQ(32, node->param(1));
    
    node = Bytecodes<kReturn>::New(arean_);
    ASSERT_NE(nullptr, node);
    ASSERT_EQ(BytecodeType::N, node->kind());
}

TEST_F(BytecodeTest, Parsing) {
    auto node = Bytecodes<kLdar64>::New(arean_, 16);
    auto instr = node->To();
    ASSERT_EQ(0x1000010, instr);
    
    node = BytecodeNode::From(arean_, instr);
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(kLdar64, node->id());
    EXPECT_EQ(BytecodeType::A, node->kind());
    EXPECT_EQ(0x10, node->param(0));
    
    node = Bytecodes<kLdaPropertyPtr>::New(arean_, 16, 32);
    instr = node->To();
    ASSERT_NE(0, instr);
    node = BytecodeNode::From(arean_, instr);
    EXPECT_EQ(kLdaPropertyPtr, node->id());
    EXPECT_EQ(BytecodeType::AB, node->kind());
    EXPECT_EQ(0x10, node->param(0));
    EXPECT_EQ(0x20, node->param(1));
}

TEST_F(BytecodeTest, Builder) {
    BytecodeArrayBuilder builder(arean_);
    
    EXPECT_EQ(0, builder.pc());
    builder.Add<kLdaf32>(0);
    builder.Add<kLdaPropertyPtr>(0, 1);
    builder.Add<kReturn>();
    EXPECT_EQ(3, builder.pc());
    builder.Add<kLdaZero>();
    
    auto instrs = builder.Build();
    EXPECT_EQ(4, instrs.size());
}

TEST_F(BytecodeTest, BuilderGoto) {
    BytecodeArrayBuilder builder(arean_);
    
    builder.Add<kLdar32>(0); // 0
    builder.Add<kLdar32>(1); // 1
    BytecodeLabel label;
    builder.Goto(&label, 0);    // 2
    builder.Add<kAdd32>(0, 1); // 3
    builder.Bind(&label);
    builder.Add<kReturn>(); // 4
    
    auto instrs = builder.Build();
    auto node = BytecodeNode::From(arean_, instrs[2]);
    ASSERT_EQ(kGoto, node->id());
    ASSERT_EQ(0, node->param(0));
    ASSERT_EQ(4, node->param(1));
}

}

}
