#include "asm/x64/asm-x64.h"
#include "asm/utils.h"
#include "mai/allocator.h"
#include "mai/env.h"
#include "gtest/gtest.h"
#include <numeric>

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
    
#define __(ins, ...) asm_.Emit_##ins(__VA_ARGS__)
    
TEST_F(X64AssemblerTest, Sanity) {
    asm_.Emit_movq(rax, 999);
    asm_.Emit_ret(0);
    
    auto fn = MakeFunction<int ()>();
    ASSERT_EQ(999, fn());
}

TEST_F(X64AssemblerTest, FarJccJump) {
    //asm_.EmitBreakpoint();
    asm_.Emit_cmpl(kRegArgv[0], kRegArgv[1]);
    Label label;
    asm_.Emit_jcc(Equal, &label, true);
    asm_.Emit_movl(rax, kRegArgv[1]);
    asm_.Emit_ret(0);
    asm_.Bind(&label);
    asm_.Emit_movl(rax, kRegArgv[0]);
    asm_.Emit_ret(0);
    
    auto fn = MakeFunction<int (int, int)>();
    ASSERT_EQ(1, fn(1, 1));
    ASSERT_EQ(2, fn(1, 2));
}
    
TEST_F(X64AssemblerTest, Add) {
    asm_.Emit_addl(kRegArgv[0], kRegArgv[1]); // 255
    asm_.Emit_movq(rax, kRegArgv[0]); // rax = 255
    asm_.Emit_addl(rax, 100); // 355
    asm_.Emit_addl(rax, Operand(kRegArgv[2], 0)); // 454
    asm_.Emit_ret(0);

    auto foo = MakeFunction<int (int, int, int *)>();
    int k = 99;
    ASSERT_EQ(454, foo(122, 133, &k));
    
    asm_.Reset();
    //asm_.EmitBreakpoint();
    asm_.Emit_addl(kRegArgv[0], kRegArgv[1]);
    asm_.Emit_addl(Operand(kRegArgv[2], 0), kRegArgv[0]);
    asm_.Emit_ret(0);
    
    auto bar = MakeFunction<void (int, int, int *)>();
    k = 0;
    bar(77, 33, &k);
    ASSERT_EQ(110, k);
}

TEST_F(X64AssemblerTest, Shift) {
    asm_.Emit_movl(rax, kRegArgv[0]);
    asm_.Emit_shll(rax, 1);
    asm_.Emit_shll(rax, 2);
    asm_.Emit_movb(rcx, 1);
    asm_.Emit_shll(rax);
    asm_.Emit_ret(0);
    {
        auto foo = MakeFunction<uint32_t (uint32_t)>();
        EXPECT_EQ(16, foo(1));
    }
    
    asm_.Reset();
    //asm_.EmitBreakpoint();
    asm_.Emit_movl(rax, kRegArgv[0]);
    asm_.Emit_shrl(rax, 1);
    asm_.Emit_shrl(rax, 2);
    asm_.Emit_movb(rcx, 1);
    asm_.Emit_shrl(rax);
    asm_.Emit_ret(0);
    {
        auto foo = MakeFunction<uint32_t (uint32_t)>();
        EXPECT_EQ(1, foo(16));
    }
}

TEST_F(X64AssemblerTest, XmmMoving) {
    
    asm_.Reset();
    asm_.Emit_movsd(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movsd(xmm1, xmm0);
    asm_.Emit_movsd(Operand(kRegArgv[1], 0), xmm1);
    asm_.Emit_ret(0);
    {
        double in = 2.22221, out = 0;
        auto foo = MakeFunction<void (double *, double *)>();
        foo(&in, &out);
        ASSERT_EQ(in, out);
    }
    
    asm_.Reset();
    asm_.Emit_movss(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movss(xmm1, xmm0);
    asm_.Emit_movss(Operand(kRegArgv[1], 0), xmm1);
    asm_.Emit_ret(0);
    {
        float in = 2.22221, out = 0;
        auto foo = MakeFunction<void (float *, float *)>();
        foo(&in, &out);
        ASSERT_EQ(in, out);
    }

    asm_.Reset();
    asm_.Emit_movapd(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movapd(xmm1, xmm0);
    asm_.Emit_movapd(Operand(kRegArgv[1], 0), xmm1);
    asm_.Emit_ret(0);
    
    {
        double in[2] = {0.0001, 0.1234};
        double out[2] = {0, 0};
        auto foo = MakeFunction<void (double *, double *)>();
        foo(in, out);
        ASSERT_EQ(in[0], out[0]);
        ASSERT_EQ(in[1], out[1]);
    }
    
    asm_.Reset();
    asm_.Emit_movaps(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movaps(xmm1, xmm0);
    asm_.Emit_movaps(Operand(kRegArgv[1], 0), xmm1);
    asm_.Emit_ret(0);
    {
        float in[4] = {1.00001, 2.00002, 3.0003, 4.00004};
        float out[4] = {0, 0, 0, 0};
        auto foo = MakeFunction<void (float *, float *)>();
        foo(in, out);
        for (int i = 0; i < arraysize(in); ++i) {
            ASSERT_EQ(in[i], out[i]) << i;
        }
    }
}
    
TEST_F(X64AssemblerTest, XmmAdd) {
    asm_.Reset();
    asm_.Emit_movsd(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movsd(xmm1, Operand(kRegArgv[1], 0));
    asm_.Emit_addsd(xmm0, xmm1);
    asm_.Emit_addsd(xmm0, Operand(kRegArgv[1], 0));
    asm_.Emit_ret(0);
    {
        double a1 = 1238.1234567, a2 = 9999.1234567;
        auto foo = MakeFunction<double (double *, double *)>();
        ASSERT_NEAR(21236.4, foo(&a1, &a2), 0.1);
        //          21236.370370099998
    }
    
    asm_.Reset();
    asm_.Emit_addss(xmm0, xmm1);
    asm_.Emit_addss(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_ret(0);
    {
        float k = 123.456;
        auto foo = MakeFunction<float (float, float, float *)>();
        ASSERT_NEAR(2012.097, foo(999.876, 888.765, &k), 0.001);
        //          2012.097
    }
    
    
    asm_.Reset();
    asm_.Emit_movapd(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movapd(xmm1, Operand(kRegArgv[1], 0));
    asm_.Emit_addpd(xmm0, xmm1);
    asm_.Emit_addpd(xmm0, Operand(kRegArgv[1], 0));
    asm_.Emit_movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.Emit_ret(0);
    {
        double a1[2] = {1238.1234567, 1238.1234567};
        double a2[2] = {9999.1234567, 9999.1234567};
        auto foo = MakeFunction<void (double *, double *)>();
        foo(a1, a2);
        ASSERT_NEAR(21236.4, a2[0], 0.1);
        ASSERT_NEAR(21236.4, a2[1], 0.1);
    }
    
    asm_.Reset();
    asm_.Emit_movaps(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movaps(xmm1, Operand(kRegArgv[1], 0));
    asm_.Emit_addps(xmm0, xmm1);
    asm_.Emit_addps(xmm0, Operand(kRegArgv[1], 0));
    asm_.Emit_movaps(Operand(kRegArgv[1], 0), xmm0);
    asm_.Emit_ret(0);
    {
        float a1[4] = {999.876, 999.876, 999.876, 999.876};
        float a2[4] = {888.765, 888.765, 888.765, 888.765};
        auto foo = MakeFunction<void (float *, float *)>();
        foo(a1, a2);
        ASSERT_NEAR(2777.406, a2[0], 0.001);
        ASSERT_NEAR(2777.406, a2[1], 0.001);
        ASSERT_NEAR(2777.406, a2[2], 0.001);
        ASSERT_NEAR(2777.406, a2[3], 0.001);
    }
}

TEST_F(X64AssemblerTest, SSEConvert) {
    
    asm_.Emit_cvtss2sd(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_ret(0);
    {
        float in = 1234.5678;
        auto foo = MakeFunction<double (float *)>();
        EXPECT_NEAR(1234.57, foo(&in), 0.01);
    }
    
    asm_.Reset();
    asm_.Emit_cvtss2sil(rax, Operand(kRegArgv[0], 0));
    asm_.Emit_ret(0);
    {
        float in = 1234.5678;
        auto foo = MakeFunction<int32_t (float *)>();
        EXPECT_EQ(1235, foo(&in));
    }
    
    asm_.Reset();
    //asm_.EmitBreakpoint();
    asm_.Emit_cvtss2sil(rax, kXmmArgv[0]);
    asm_.Emit_ret(0);
    {
        auto foo = MakeFunction<int32_t (float)>();
        EXPECT_EQ(1235, foo(1234.5678));
    }
    
    asm_.Reset();
    asm_.Emit_cvtss2siq(rax, kXmmArgv[0]);
    asm_.Emit_ret(0);
    {
        auto foo = MakeFunction<int64_t (float)>();
        EXPECT_EQ(1235, foo(1234.5678));
    }
    
    asm_.Reset();
    asm_.Emit_cvttsd2sil(rax, kXmmArgv[0]);
    asm_.Emit_ret(0);
    {
        auto foo = MakeFunction<int32_t (double)>();
        EXPECT_EQ(1234, foo(1234.5678));
    }
    
    asm_.Reset();
    asm_.Emit_cvttsd2siq(rax, Operand(kRegArgv[0], 0));
    asm_.Emit_ret(0);
    {
        double k = 98765.654321;
        auto foo = MakeFunction<int64_t (double *)>();
        ASSERT_EQ(98765, foo(&k));
    }
}

TEST_F(X64AssemblerTest, SSEPackedConvert) {
    asm_.Emit_cvtdq2pd(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.Emit_ret(0);
    {
        int32_t in[2] = {1234567, 8901234};
        double out[2] = {0, 0};
        auto foo = MakeFunction<void (int32_t *, double *)>();
        foo(in, out);
        EXPECT_NEAR(1234567, out[0], 0.1);
        EXPECT_NEAR(8901234, out[1], 0.1);
    }
    
    asm_.Reset();
    asm_.Emit_cvtpd2dq(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.Emit_ret(0);
    {
        double in[2] = {1.9, 2.0};
        int32_t out[4] = {0, 0, 0, 0};
        auto foo = MakeFunction<void (double *, int32_t *)>();
        foo(in, out);
        EXPECT_EQ(2, out[0]);
        EXPECT_EQ(2, out[1]);
    }
    
    asm_.Reset();
    asm_.Emit_cvtps2pd(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.Emit_ret(0);
    {
        float in[4] = {19.5, 20.1};
        double out[2] = {0, 0};
        auto foo = MakeFunction<void (float *, double *)>();
        foo(in, out);
        EXPECT_NEAR(19.5, out[0], 0.1);
        EXPECT_NEAR(20.1, out[1], 0.1);
    }
    
    asm_.Reset();
    asm_.Emit_cvtpd2ps(xmm0, Operand(kRegArgv[0], 0));
    asm_.Emit_movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.Emit_ret(0);
    {
        double in[2] = {999.9, 888.8};
        float  out[4] = {0, 0, 0, 0};
        auto foo = MakeFunction<void (double *, float *)>();
        foo(in, out);
        EXPECT_NEAR(999.9, out[0], 0.1);
        EXPECT_NEAR(888.8, out[1], 0.1);
    }
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
    
    asm_.Emit_movq(Operand(a0, a1, times_4, 0), 999);
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
