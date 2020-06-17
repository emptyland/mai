#include "lang/simplified-code-generator.h"
#include "lang/compiler.h"
#include "lang/bytecode-array-builder.h"
#include "lang/bytecode.h"
#include "lang/machine.h"
#include "lang/value-inl.h"
#include "test/isolate-initializer.h"
#include "base/arenas.h"
#include "base/slice.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class SimplifiedCodeGeneratorTest : public test::IsolateInitializer {
public:
    
    
    void MakeSanity(std::unique_ptr<CompilationInfo> *info) {
        BytecodeArrayBuilder builder(&arena_);
        BytecodeLabel lable;
        builder.Bind(&lable);
        builder.Add<kLdaSmi32>(1);
        builder.Add<kStar32>(GetStackOffset(68));
        builder.Jump(&lable, 0);
        
        info->reset(Build(std::vector<Span32>(1)/*const_pool*/,
                          builder.Build()/*bytecodes_sequence*/, {5,6,7}/*associated_pc*/,
                          {}/*invoke_info*/, 72/*stack_size*/));
    }
    
    void MakeConstantPoolOps(std::unique_ptr<CompilationInfo> *info) {
        BytecodeArrayBuilder builder(&arena_);
        BytecodeLabel lable;
        builder.Bind(&lable);
        builder.Add<kLdaConst32>(GetConstOffset(0));
        builder.Add<kStar32>(GetStackOffset(68));
        builder.Add<kLdaConstPtr>(GetConstOffset(32));
        builder.Add<kStar32>(GetStackOffset(72));
        builder.Jump(&lable, 0);

        info->reset(Build(std::vector<Span32>(2)/*const_pool*/,
                          builder.Build()/*bytecodes_sequence*/, {4,5,6,7,8}/*associated_pc*/,
                          {}/*invoke_info*/, 88/*stack_size*/));
    }
    
    void MakeComparationOps(std::unique_ptr<CompilationInfo> *info) {
        BytecodeArrayBuilder builder(&arena_);
        BytecodeLabel retry;
        builder.Bind(&retry);
        builder.Add<kTestLessThan32>(GetStackOffset(68), GetStackOffset(72));
        BytecodeLabel exit;
        builder.JumpIfTrue(&exit, 0);
        builder.Add<kIncrement32>(GetStackOffset(68), 1);
        builder.Jump(&retry, 0);
        builder.Bind(&exit);
        
        info->reset(Build(std::vector<Span32>(1)/*const_pool*/,
                          builder.Build()/*bytecodes_sequence*/, {5,6,7,8}/*associated_pc*/,
                          {}/*invoke_info*/, 88/*stack_size*/));
    }

    void MakeInlineCallingSanity(std::unique_ptr<CompilationInfo> *info) {
        BytecodeArrayBuilder builder(&arena_);
        BytecodeLabel lable;
        builder.Bind(&lable);
        /*[4]*/ builder.Add<kLdaGlobalPtr>(GetGlobalOffset(0));
        /*[5]*/ builder.Add<kLdaSmi32>(1);
        /*[6]*/ builder.Add<kStar32>(72);
        /*[7]*/ builder.Add<kCallBytecodeFunction>(0, 4);
        /*[0]*/ builder.Add<kCheckStack>();
        /*[1]*/ builder.Add<kLdaSmi32>(2);
        /*[2]*/ builder.Add<kReturn>();
        /*[8]*/ builder.Jump(&lable, 0);
        
        auto bytecodes = builder.Build();
        Function *start = BuildFunction("start", std::vector<Span32>(1), bytecodes, 88);
        Function *inner = BuildFunction("inner", std::vector<Span32>(1), bytecodes, 64);
        
        std::map<size_t, CompilationInfo::InvokeInfo> invoke_info =
            {std::make_pair(4, CompilationInfo::InvokeInfo{inner, 0, 4})};
        //invoke_info.insert(std::make_pair(4, CompilationInfo::InvokeInfo{inner, 0, 4}));

        info->reset(new CompilationInfo(Machine::This(), nullptr, start, 0/*start_slot*/,
                                        4/*star_pc*/, std::move(bytecodes),
                                        {4,5,6,7,0,1,2,8}/*associated_pc*/, std::move(invoke_info)));
    }
    
    CompilationInfo *Build(const std::vector<Span32> &const_pool,
                           std::vector<BytecodeInstruction> &&bytecode_sequence,
                           std::vector<uint32_t> &&associated_pc,
                           std::map<size_t, CompilationInfo::InvokeInfo> &&invoke_info,
                           uint32_t stack_size) {
        BytecodeArray *bytecodes = isolate_->metadata_space()->NewBytecodeArray(bytecode_sequence);
        
        Function *fun = FunctionBuilder("fun")
            .kind(Function::BYTECODE)
            .prototype({}, false, kType_void)
            .bytecode(bytecodes)
            .stack_bitmap({0})
            .stack_size(RoundUp(stack_size, kStackAligmentSize))
            .const_pool(const_pool)
            .const_pool_bitmap({0})
        .Build(isolate_->metadata_space());

        return new CompilationInfo(Machine::This(), nullptr, fun, 0/*start_slot*/,
                                   associated_pc.front()/*star_pc*/, std::move(bytecode_sequence),
                                   std::move(associated_pc), std::move(invoke_info));
    }
    
    Function *BuildFunction(const std::string &name,
                            const std::vector<Span32> &const_pool,
                            const std::vector<BytecodeInstruction> &bytecodes_sequence,
                            uint32_t stack_size) {
        BytecodeArray *bytecodes = isolate_->metadata_space()->NewBytecodeArray(bytecodes_sequence);
        return FunctionBuilder(name)
            .kind(Function::BYTECODE)
            .prototype({}, false, kType_void)
            .bytecode(bytecodes)
            .stack_bitmap({0})
            .stack_size(RoundUp(stack_size, kStackAligmentSize))
            .const_pool(const_pool)
            .const_pool_bitmap({0})
        .Build(isolate_->metadata_space());
    }

    base::StandaloneArena arena_;
}; // class SimplifiedCodeGeneratorTest

TEST_F(SimplifiedCodeGeneratorTest, Sanity) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    
    std::unique_ptr<CompilationInfo> info;
    MakeSanity(&info);
    
    Local<Kode> code(GenerateSimplifiedCode(info.get(), false, true, &arena_));
    ASSERT_TRUE(code.is_value_not_null());
    ASSERT_EQ(1, code->optimization_level());
    ASSERT_EQ(Code::OPTIMIZATION, code->kind());
    EXPECT_GT(code->size(), 0);
}

TEST_F(SimplifiedCodeGeneratorTest, ConstantPoolOps) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    
    std::unique_ptr<CompilationInfo> info;
    MakeConstantPoolOps(&info);

    Local<Kode> code(GenerateSimplifiedCode(info.get(), false, true, &arena_));
    ASSERT_TRUE(code.is_value_not_null());
    ASSERT_EQ(1, code->optimization_level());
    ASSERT_EQ(Code::OPTIMIZATION, code->kind());
    EXPECT_GT(code->size(), 0);
}

TEST_F(SimplifiedCodeGeneratorTest, ComparationOps) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    
    std::unique_ptr<CompilationInfo> info;
    MakeComparationOps(&info);

    Local<Kode> code(GenerateSimplifiedCode(info.get(), false, true, &arena_));
    ASSERT_TRUE(code.is_value_not_null());
    ASSERT_EQ(1, code->optimization_level());
    ASSERT_EQ(Code::OPTIMIZATION, code->kind());
    EXPECT_GT(code->size(), 0);
}

TEST_F(SimplifiedCodeGeneratorTest, InlineCallingSanity) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    
    std::unique_ptr<CompilationInfo> info;
    MakeInlineCallingSanity(&info);

    Local<Kode> code(GenerateSimplifiedCode(info.get(), false, true, &arena_));
    ASSERT_TRUE(code.is_value_not_null());
    ASSERT_EQ(1, code->optimization_level());
    ASSERT_EQ(Code::OPTIMIZATION, code->kind());
    EXPECT_GT(code->size(), 0);
}

} // namespace lang

} // namespace mai
