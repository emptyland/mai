#include "lang/bytecode.h"
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
    ASSERT_EQ(kBytecode_TypeA, node->kind());
    ASSERT_EQ(16, node->param(0));
    
    node = Bytecodes<kLdaPropertyPtr>::New(arean_, 16, 32);
    ASSERT_NE(nullptr, node);
    ASSERT_EQ(kBytecode_TypeAB, node->kind());
    ASSERT_EQ(16, node->param(0));
    ASSERT_EQ(32, node->param(1));
    
    node = Bytecodes<kReturn>::New(arean_);
    ASSERT_NE(nullptr, node);
    ASSERT_EQ(kBytecode_TypeN, node->kind());
}

TEST_F(BytecodeTest, Parsing) {
    auto node = Bytecodes<kLdar64>::New(arean_, 16);
    auto instr = node->To();
    ASSERT_EQ(0x10, instr);
    
    node = BytecodeNode::From(arean_, instr);
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(kLdar64, node->id());
    EXPECT_EQ(kBytecode_TypeA, node->kind());
    EXPECT_EQ(0x10, node->param(0));
    
    node = Bytecodes<kLdaPropertyPtr>::New(arean_, 16, 32);
    instr = node->To();
    ASSERT_NE(0, instr);
    node = BytecodeNode::From(arean_, instr);
    EXPECT_EQ(kLdaPropertyPtr, node->id());
    EXPECT_EQ(kBytecode_TypeAB, node->kind());
    EXPECT_EQ(0x10, node->param(0));
    EXPECT_EQ(0x20, node->param(1));
}

}

}
