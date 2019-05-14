#include "asm/x64/asm-x64.h"
#include "mai/allocator.h"
#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {

namespace x64 {

class X64AssemblerTest : public ::testing::Test {
public:
    X64AssemblerTest() {}
    
    ~X64AssemblerTest() override {
        auto lla = env->GetLowLevelAllocator();
        for (auto blk : blks_) {
            lla->Free(blk, lla->granularity());
        }
    }
    
    template<class T>
    T *MakeFunction() {
        auto lla = env->GetLowLevelAllocator();
        auto blk = lla->Allocate(asm_.buf().size());
        auto rs = lla->SetAccess(blk, asm_.buf().size(), Allocator::kEx|Allocator::kWr);
        if (!rs) {
            return nullptr;
        }
        ::memcpy(blk, asm_.buf().data(), asm_.buf().size());
        blks_.push_back(blk);
        rs = lla->SetAccess(blk, asm_.buf().size(), Allocator::kEx|Allocator::kRd);
        if (!rs) {
            return nullptr;
        }
        return reinterpret_cast<T *>(blk);
    }

    Env *env = Env::Default();
    Assembler asm_;
    std::vector<void *> blks_;
private:
    
};
    
TEST_F(X64AssemblerTest, Sanity) {
    asm_.Emit_movq(rax, 999);
    asm_.Emit_ret(0);
    
    auto fn = MakeFunction<int ()>();
    ASSERT_EQ(999, fn());
}

TEST_F(X64AssemblerTest, FarJccJump) {
    asm_.Emit_test(kRegArgv[0], kRegArgv[1]);
    Label label;
    asm_.Emit_jcc(Equal, &label, true);
    asm_.Emit_movq(rax, kRegArgv[0]);
    asm_.Emit_ret(0);
    asm_.Bind(&label);
    asm_.Emit_movq(rax, kRegArgv[1]);
    asm_.Emit_ret(0);
    
    auto fn = MakeFunction<int (int, int)>();
    ASSERT_EQ(1, fn(1, 1));
    ASSERT_EQ(2, fn(1, 2));
}
    
static int TestStub1(int a, int b) {
    return a + b;
}

TEST_F(X64AssemblerTest, CallOutsideFunction) {
    asm_.Emit_pushq(rcx);
    asm_.Emit_movq(rcx, reinterpret_cast<Address>(TestStub1));
    asm_.Emit_movq(kRegArgv[0], 1000);
    asm_.Emit_movq(kRegArgv[1], -1);
    asm_.Emit_call(rcx);
    asm_.Emit_popq(rcx);
    asm_.Emit_ret(0);
    asm_.Emit_nop();
    
    auto fn = MakeFunction<int ()>();
    ASSERT_EQ(999, fn());
}
    
struct TestFoo1 {
    int a;
    int b;
    int c;
};
    
TEST_F(X64AssemblerTest, SetStruct) {
    
}

} // namespace x64
    
} // namespace mai
