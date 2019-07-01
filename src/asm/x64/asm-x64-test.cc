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
    
#define __ asm_.
    
TEST_F(X64AssemblerTest, Sanity) {
    asm_.movq(rax, 999);
    asm_.ret(0);
    
    auto fn = MakeFunction<int ()>();
    ASSERT_EQ(999, fn());
}

TEST_F(X64AssemblerTest, FarJccJump) {
    //asm_.EmitBreakpoint();
    asm_.cmpl(kRegArgv[0], kRegArgv[1]);
    Label label;
    asm_.j(Equal, &label, false);
    asm_.movl(rax, kRegArgv[1]);
    asm_.ret(0);
    asm_.Bind(&label);
    asm_.movl(rax, kRegArgv[0]);
    asm_.ret(0);
    
    auto fn = MakeFunction<int (int, int)>();
    ASSERT_EQ(1, fn(1, 1));
    ASSERT_EQ(2, fn(1, 2));
}
    
TEST_F(X64AssemblerTest, Add) {
    asm_.addl(kRegArgv[0], kRegArgv[1]); // 255
    asm_.movq(rax, kRegArgv[0]); // rax = 255
    asm_.addl(rax, 100); // 355
    asm_.addl(rax, Operand(kRegArgv[2], 0)); // 454
    asm_.ret(0);

    auto foo = MakeFunction<int (int, int, int *)>();
    int k = 99;
    ASSERT_EQ(454, foo(122, 133, &k));
    
    asm_.Reset();
    //asm_.EmitBreakpoint();
    asm_.addl(kRegArgv[0], kRegArgv[1]);
    asm_.addl(Operand(kRegArgv[2], 0), kRegArgv[0]);
    asm_.ret(0);
    
    auto bar = MakeFunction<void (int, int, int *)>();
    k = 0;
    bar(77, 33, &k);
    ASSERT_EQ(110, k);
}

TEST_F(X64AssemblerTest, Shift) {
    asm_.movl(rax, kRegArgv[0]);
    asm_.shll(rax, 1);
    asm_.shll(rax, 2);
    asm_.movb(rcx, 1);
    asm_.shll(rax);
    asm_.ret(0);
    {
        auto foo = MakeFunction<uint32_t (uint32_t)>();
        EXPECT_EQ(16, foo(1));
    }
    
    asm_.Reset();
    //asm_.EmitBreakpoint();
    asm_.movl(rax, kRegArgv[0]);
    asm_.shrl(rax, 1);
    asm_.shrl(rax, 2);
    asm_.movb(rcx, 1);
    asm_.shrl(rax);
    asm_.ret(0);
    {
        auto foo = MakeFunction<uint32_t (uint32_t)>();
        EXPECT_EQ(1, foo(16));
    }
}

TEST_F(X64AssemblerTest, UnsignedDiv) {
    
    __ Reset();
    __ movl(rax, kRegArgv[0]);
    __ movq(r13, kRegArgv[2]);
    __ xorl(rdx, rdx);
    __ divl(kRegArgv[1]);
    __ movl(Operand(r13, 0), rdx);
    __ ret(0);
    {
        uint32_t a = 29, b = 3, c = 0;
        auto foo = MakeFunction<uint32_t (uint32_t, uint32_t, uint32_t *)>();
        EXPECT_EQ(a / b, foo(a, b, &c));
        EXPECT_EQ(a % b, c);
    }
    
    __ Reset();
    __ movq(rax, kRegArgv[0]);
    __ movq(r13, kRegArgv[2]);
    __ xorq(rdx, rdx);
    __ divq(kRegArgv[1]);
    __ movq(Operand(r13, 0), rdx);
    __ ret(0);
    {
        uint64_t a = 29, b = 3, c = 0;
        auto foo = MakeFunction<uint64_t (uint64_t, uint64_t, uint64_t *)>();
        EXPECT_EQ(a / b, foo(a, b, &c));
        EXPECT_EQ(a % b, c);
    }
}

TEST_F(X64AssemblerTest, UnsignedMul) {
    
    __ Reset();
    __ movl(rax, kRegArgv[0]);
    __ mull(kRegArgv[1]);
    __ ret(0);
    {
        uint32_t a = 99999, b = 999;
        auto foo = MakeFunction<uint32_t(uint32_t, uint32_t)>();
        EXPECT_EQ(a * b, foo(a, b));
    }
    
    __ Reset();
    __ movq(rax, kRegArgv[0]);
    __ mulq(kRegArgv[1]);
    __ ret(0);
    {
        uint64_t a = 999999, b = 99999;
        auto foo = MakeFunction<uint64_t(uint64_t, uint64_t)>();
        EXPECT_EQ(a * b, foo(a, b));
    }
}

TEST_F(X64AssemblerTest, XmmMoving) {
    
    asm_.Reset();
    asm_.movsd(xmm0, Operand(kRegArgv[0], 0));
    asm_.movsd(xmm1, xmm0);
    asm_.movsd(Operand(kRegArgv[1], 0), xmm1);
    asm_.ret(0);
    {
        double in = 2.22221, out = 0;
        auto foo = MakeFunction<void (double *, double *)>();
        foo(&in, &out);
        ASSERT_EQ(in, out);
    }
    
    asm_.Reset();
    asm_.movss(xmm0, Operand(kRegArgv[0], 0));
    asm_.movss(xmm1, xmm0);
    asm_.movss(Operand(kRegArgv[1], 0), xmm1);
    asm_.ret(0);
    {
        float in = 2.22221, out = 0;
        auto foo = MakeFunction<void (float *, float *)>();
        foo(&in, &out);
        ASSERT_EQ(in, out);
    }

    asm_.Reset();
    asm_.movapd(xmm0, Operand(kRegArgv[0], 0));
    asm_.movapd(xmm1, xmm0);
    asm_.movapd(Operand(kRegArgv[1], 0), xmm1);
    asm_.ret(0);
    
    {
        double in[2] = {0.0001, 0.1234};
        double out[2] = {0, 0};
        auto foo = MakeFunction<void (double *, double *)>();
        foo(in, out);
        ASSERT_EQ(in[0], out[0]);
        ASSERT_EQ(in[1], out[1]);
    }
    
    asm_.Reset();
    asm_.movaps(xmm0, Operand(kRegArgv[0], 0));
    asm_.movaps(xmm1, xmm0);
    asm_.movaps(Operand(kRegArgv[1], 0), xmm1);
    asm_.ret(0);
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
    asm_.movsd(xmm0, Operand(kRegArgv[0], 0));
    asm_.movsd(xmm1, Operand(kRegArgv[1], 0));
    asm_.addsd(xmm0, xmm1);
    asm_.addsd(xmm0, Operand(kRegArgv[1], 0));
    asm_.ret(0);
    {
        double a1 = 1238.1234567, a2 = 9999.1234567;
        auto foo = MakeFunction<double (double *, double *)>();
        ASSERT_NEAR(21236.4, foo(&a1, &a2), 0.1);
        //          21236.370370099998
    }
    
    asm_.Reset();
    asm_.addss(xmm0, xmm1);
    asm_.addss(xmm0, Operand(kRegArgv[0], 0));
    asm_.ret(0);
    {
        float k = 123.456;
        auto foo = MakeFunction<float (float, float, float *)>();
        ASSERT_NEAR(2012.097, foo(999.876, 888.765, &k), 0.001);
        //          2012.097
    }
    
    
    asm_.Reset();
    asm_.movapd(xmm0, Operand(kRegArgv[0], 0));
    asm_.movapd(xmm1, Operand(kRegArgv[1], 0));
    asm_.addpd(xmm0, xmm1);
    asm_.addpd(xmm0, Operand(kRegArgv[1], 0));
    asm_.movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.ret(0);
    {
        double a1[2] = {1238.1234567, 1238.1234567};
        double a2[2] = {9999.1234567, 9999.1234567};
        auto foo = MakeFunction<void (double *, double *)>();
        foo(a1, a2);
        ASSERT_NEAR(21236.4, a2[0], 0.1);
        ASSERT_NEAR(21236.4, a2[1], 0.1);
    }
    
    asm_.Reset();
    asm_.movaps(xmm0, Operand(kRegArgv[0], 0));
    asm_.movaps(xmm1, Operand(kRegArgv[1], 0));
    asm_.addps(xmm0, xmm1);
    asm_.addps(xmm0, Operand(kRegArgv[1], 0));
    asm_.movaps(Operand(kRegArgv[1], 0), xmm0);
    asm_.ret(0);
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
    
    asm_.cvtss2sd(xmm0, Operand(kRegArgv[0], 0));
    asm_.ret(0);
    {
        float in = 1234.5678;
        auto foo = MakeFunction<double (float *)>();
        EXPECT_NEAR(1234.57, foo(&in), 0.01);
    }
    
    asm_.Reset();
    asm_.cvtss2sil(rax, Operand(kRegArgv[0], 0));
    asm_.ret(0);
    {
        float in = 1234.5678;
        auto foo = MakeFunction<int32_t (float *)>();
        EXPECT_EQ(1235, foo(&in));
    }
    
    asm_.Reset();
    //asm_.EmitBreakpoint();
    asm_.cvtss2sil(rax, kXmmArgv[0]);
    asm_.ret(0);
    {
        auto foo = MakeFunction<int32_t (float)>();
        EXPECT_EQ(1235, foo(1234.5678));
    }
    
    asm_.Reset();
    asm_.cvtss2siq(rax, kXmmArgv[0]);
    asm_.ret(0);
    {
        auto foo = MakeFunction<int64_t (float)>();
        EXPECT_EQ(1235, foo(1234.5678));
    }
    
    asm_.Reset();
    asm_.cvttsd2sil(rax, kXmmArgv[0]);
    asm_.ret(0);
    {
        auto foo = MakeFunction<int32_t (double)>();
        EXPECT_EQ(1234, foo(1234.5678));
    }
    
    asm_.Reset();
    asm_.cvttsd2siq(rax, Operand(kRegArgv[0], 0));
    asm_.ret(0);
    {
        double k = 98765.654321;
        auto foo = MakeFunction<int64_t (double *)>();
        ASSERT_EQ(98765, foo(&k));
    }
}

TEST_F(X64AssemblerTest, SSEPackedConvert) {
    asm_.cvtdq2pd(xmm0, Operand(kRegArgv[0], 0));
    asm_.movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.ret(0);
    {
        int32_t in[2] = {1234567, 8901234};
        double out[2] = {0, 0};
        auto foo = MakeFunction<void (int32_t *, double *)>();
        foo(in, out);
        EXPECT_NEAR(1234567, out[0], 0.1);
        EXPECT_NEAR(8901234, out[1], 0.1);
    }
    
    asm_.Reset();
    asm_.cvtpd2dq(xmm0, Operand(kRegArgv[0], 0));
    asm_.movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.ret(0);
    {
        double in[2] = {1.9, 2.0};
        int32_t out[4] = {0, 0, 0, 0};
        auto foo = MakeFunction<void (double *, int32_t *)>();
        foo(in, out);
        EXPECT_EQ(2, out[0]);
        EXPECT_EQ(2, out[1]);
    }
    
    asm_.Reset();
    asm_.cvtps2pd(xmm0, Operand(kRegArgv[0], 0));
    asm_.movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.ret(0);
    {
        float in[4] = {19.5, 20.1};
        double out[2] = {0, 0};
        auto foo = MakeFunction<void (float *, double *)>();
        foo(in, out);
        EXPECT_NEAR(19.5, out[0], 0.1);
        EXPECT_NEAR(20.1, out[1], 0.1);
    }
    
    asm_.Reset();
    asm_.cvtpd2ps(xmm0, Operand(kRegArgv[0], 0));
    asm_.movapd(Operand(kRegArgv[1], 0), xmm0);
    asm_.ret(0);
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
    asm_.pushq(rcx);
    asm_.movq(rcx, reinterpret_cast<Address>(TestStub1));
    asm_.movq(kRegArgv[0], 1000);
    asm_.movq(kRegArgv[1], -1);
    asm_.call(rcx);
    asm_.popq(rcx);
    asm_.ret(0);
    asm_.nop();
    
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
    asm_.movq(rax, 1);
    asm_.movq(Operand(kRegArgv[0], kOffsetA), rax);
    asm_.movq(rax, 2);
    asm_.movq(Operand(kRegArgv[0], kOffsetB), rax);
    asm_.movq(rax, 3);
    asm_.movq(Operand(kRegArgv[0], kOffsetC), rax);
    asm_.ret(0);
    
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
    
    asm_.movq(Operand(a0, a1, times_4, 0), 999);
    asm_.ret(0);

    int d[4] = {0, 0, 0, 0};
    auto fn = MakeFunction<void (int *, int)>();
    fn(d, 0);
    fn(d, 3);
    EXPECT_EQ(d[0], 999);
    EXPECT_EQ(d[1], 0);
    EXPECT_EQ(d[2], 0);
    EXPECT_EQ(d[3], 999);
}
    
TEST_F(X64AssemblerTest, Loop) {
    Label out, retry;
    
    //__ Breakpoint();
    __ movl(rax, 0);
    __ movl(rcx, kRegArgv[0]);
    __ Bind(&retry);
    __ cmpl(rcx, 0);
    __ UnlikelyJ(BelowEqual, &out, true);
    __ addl(rax, 10);
    __ decl(rcx);
    __ jmp(&retry, true);
    __ Bind(&out);
    __ ret(0);
    
    auto fn = MakeFunction<int (int)>();
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(i * 10, fn(i));
    }
}

class TestCallingStub {
public:
    TestCallingStub(int a, int b) : a_(a), b_(b) {}

    int call0() {
        return a_ + b_;
    }
    
private:
    int a_ = 0;
    int b_ = 0;
};
    
TEST_F(X64AssemblerTest, CallCxxMethod) {
    auto fp = &TestCallingStub::call0;
    //printf("%lu\n", sizeof(fp));
    Address stub[2];
    ::memcpy(stub, &fp, sizeof(fp));
    //printf("%p, %p\n", stub[0], stub[1]);
    
    __ Reset();
    __ call(Operand(kRegArgv[1], 0));
    __ ret(0);
    {
        TestCallingStub ob(1, 2);
        auto foo = MakeFunction<int (TestCallingStub *, Address)>();
        EXPECT_EQ(3, foo(&ob, reinterpret_cast<Address>(&stub[0])));
    }
}

TEST_F(X64AssemblerTest, RIP) {
    __ Reset();
    __ nop();
    __ lea(rax, Operand(rip, 0));
    __ ret(0);
    {
        auto foo = MakeFunction<intptr_t ()>();
        ASSERT_EQ(8, foo() - reinterpret_cast<intptr_t>(foo));
    }
}
    
//static void TestStubThrowException() {
//    printf("raise\n");
//    throw 123;
//}
//
//TEST_F(X64AssemblerTest, CallThrowCxxException) {
//    __ Reset();
//    __ pushq(rbp);
//    __ movq(rbp, rsp);
//    __ call(kRegArgv[0]);
//    __ popq(rbp);
//    __ ret(0);
//    {
//        auto foo = MakeFunction<void (Address)>();
//        try {
//            foo(reinterpret_cast<Address>(&TestStubThrowException));
//            //TestStubThrowException();
//        } catch (int n) {
//            printf("catch!\n");
//        }
//    }
//}

} // namespace x64
    
} // namespace mai