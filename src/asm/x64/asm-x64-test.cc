#include "asm/x64/asm-x64.h"
#include "asm/utils.h"
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
    //asm_.EmitBreakpoint();
    asm_.Emit_cmp(kRegArgv[0], kRegArgv[1]);
    Label label;
    asm_.Emit_jcc(Equal, &label, true);
    asm_.Emit_movq(rax, kRegArgv[1]);
    asm_.Emit_ret(0);
    asm_.Bind(&label);
    asm_.Emit_movq(rax, kRegArgv[0]);
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
    int a = 0;
    int b = 0;
    int c = 0;
    
    static const size_t kOffsetA;
};

const size_t TestFoo1::kOffsetA = arch::ObjectTemplate<TestFoo1>::OffsetOf(&TestFoo1::a);

    
TEST_F(X64AssemblerTest, SetStruct) {
    arch::ObjectTemplate<TestFoo1, int> foo_tmpl;
    static const int kOffsetA = foo_tmpl.Offset(&TestFoo1::a);
    static const int kOffsetB = foo_tmpl.Offset(&TestFoo1::b);
    static const int kOffsetC = foo_tmpl.Offset(&TestFoo1::c);

    //asm_.EmitBreakpoint();
    asm_.Emit_movq(rax, 1);
    asm_.Emit_movq(Operand(kRegArgv[0], kOffsetA), rax);
    asm_.Emit_movq(rax, 2);
    asm_.Emit_movq(Operand(kRegArgv[0], kOffsetB), rax);
    asm_.Emit_movq(rax, 3);
    asm_.Emit_movq(Operand(kRegArgv[0], kOffsetC), rax);
    asm_.Emit_ret(0);
    
    TestFoo1 foo;
    auto fn = MakeFunction<void (TestFoo1 *)>();
    fn(&foo);
    
    ASSERT_EQ(1, foo.a);
    ASSERT_EQ(2, foo.b);
    ASSERT_EQ(3, foo.c);
}
    
TEST_F(X64AssemblerTest, SetArray) {
    Register a0 = kRegArgv[0];
    Register a1 = kRegArgv[1];
    
    asm_.Emit_movq(Operand(a0, a1, times_4, 0), Immediate{999});
    asm_.Emit_ret(0);
    
    int d[4] = {0, 0, 0, 0};
    auto fn = MakeFunction<void (int *, int)>();
    fn(d, 0);
    fn(d, 3);
    EXPECT_EQ(d[0], 999);
    EXPECT_EQ(d[1], 0);
    EXPECT_EQ(d[2], 0);
    EXPECT_EQ(d[3], 999);
}

} // namespace x64
    
} // namespace mai
