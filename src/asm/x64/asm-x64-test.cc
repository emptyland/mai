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

#if defined(DEBUG) || defined(_DEBUG)
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
#endif // #if defined(DEBUG) || defined(_DEBUG)

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

#if defined(DEBUG) || defined(_DEBUG)
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
#endif // #if defined(DEBUG) || defined(_DEBUG)
    
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
    __ leaq(rax, Operand(rip, 0));
    __ ret(0);
    {
        auto foo = MakeFunction<intptr_t ()>();
        ASSERT_EQ(8, foo() - reinterpret_cast<intptr_t>(foo));
    }
}

int StubDemoFunction(int a, int b) {
    //printf("hit it!\n");
    return a + b;
}

TEST_F(X64AssemblerTest, CallStub) {
    // Call this stub code
    __ Reset();
    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ subq(rsp, 16);

    __ movl(Operand(rbp, -4), kRegArgv[0]);
    __ movl(Operand(rbp, -8), kRegArgv[1]);
    
    Label stub_label;
    __ call(&stub_label);

    __ addq(rsp, 16);
    __ popq(rbp);
    __ ret(0);
    
    // Make Stub code
    __ Bind(&stub_label);
    __ pushq(rbp);
    __ movq(rbp, rsp);
    
    __ movl(kRegArgv[0], Operand(rbp, 28));
    __ movl(kRegArgv[1], Operand(rbp, 24));
    __ movq(rbx, reinterpret_cast<Address>(&StubDemoFunction));
    __ call(rbx);

    __ popq(rbp);
    __ ret(0);
    
    auto foo = MakeFunction<int(int, int)>();
    ASSERT_EQ(666, foo(233, 433));
    ASSERT_EQ(3, foo(1, 2));
}

class DemoHeaderStruct {
public:
    DemoHeaderStruct(int capacity)
    : capacity_(capacity)
    , len_(0) {
        ::memset(buf_, 0, sizeof(int) * capacity_);
    }
    
    int buf(int i) const { return buf_[i]; }
    
    static DemoHeaderStruct *New(int capacity) {
        size_t n = sizeof(DemoHeaderStruct) + capacity * sizeof(int);
        void *chunk = ::malloc(n);
        return new (chunk) DemoHeaderStruct(capacity);
    }
private:
    int capacity_;
    int len_;
    int buf_[0];
};

TEST_F(X64AssemblerTest, ComplexAddressAccessWay) {
    __ Reset();
    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ movq(Operand(kRegArgv[0], kRegArgv[1], times_4, sizeof(DemoHeaderStruct)), kRegArgv[2]);
    __ popq(rbp);
    __ ret(0);
    
    auto demo = DemoHeaderStruct::New(16);
    auto foo = MakeFunction<void (DemoHeaderStruct *, int, int)>();

    foo(demo, 0, 233);
    ASSERT_EQ(233, demo->buf(0));
    
    foo(demo, 1, 433);
    ASSERT_EQ(433, demo->buf(1));
    
    foo(demo, 15, 333);
    ASSERT_EQ(333, demo->buf(15));
}

// Return the smallest multiple of m which is >= x.
// (x + m - 1) & -m
TEST_F(X64AssemblerTest, RoundUp) {
    // x: Argv_0, m: Argv_1
    __ Reset();
    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ movl(rax, Argv_0);
    __ addl(rax, Argv_1); // x + m
    __ decl(rax); // - 1
    __ negl(Argv_1); // -m
    __ andl(rax, Argv_1);
    __ popq(rbp);
    __ ret(0);
    
    auto foo = MakeFunction<int (int, int)>();
    ASSERT_EQ(16, foo(1, 16));
    ASSERT_EQ(16, foo(16, 16));
    ASSERT_EQ(32, foo(17, 16));
    ASSERT_EQ(32, foo(32, 16));
    ASSERT_EQ(48, foo(33, 16));
}

TEST_F(X64AssemblerTest, BytecodeDecode) {
    __ Reset();
    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ subq(rsp, 16);
    __ movl(Operand(rbp, -4), 1);
    __ movl(Operand(rbp, -8), 2);
    __ movl(Operand(rbp, -12), 3);
    __ movl(Operand(rbp, -16), 4);
    
    __ movl(rbx, Argv_0);
    __ andl(rbx, 0xfff000);
    __ shrl(rbx, 12);
    __ negq(rbx);
    __ movl(rax, Operand(rbp, rbx, times_1, 0));
    
    __ movl(rbx, Argv_0);
    __ andl(rbx, 0xfff);
    __ negq(rbx);
    
    __ addl(rax, Operand(rbp, rbx, times_1, 0));
    
    __ addq(rsp, 16);
    __ popq(rbp);
    __ ret(0);
    
    auto foo = MakeFunction<int (uint32_t)>();
    ASSERT_EQ(2, foo(0x00004004));
    ASSERT_EQ(3, foo(0x00008004));
}

TEST_F(X64AssemblerTest, SSE42strcmpDemo) {
    __ Reset();
    __ pushq(rbp);
    __ movq(rbp, rsp);

    __ movl(rax, 8);
    __ movl(rdx, 8);
    __ movdqu(xmm0, Operand(Argv_0, 0));
    __ pcmpestri(xmm0, Operand(Argv_1, 0), PCMP::EqualEach|PCMP::NegativePolarity);
    
    Label if_false;
    __ j(Carry, &if_false, false);
    __ movl(rax, 1);
    __ popq(rbp);
    __ ret(0);
    
    __ Bind(&if_false);
    __ xorq(rax, rax);
    __ popq(rbp);
    __ ret(0);
    
    char s1[17] = {"01234567"};
    char s2[17] = {"01234567"};
    auto foo = MakeFunction<int (const char *, const char *)>();
    
    ASSERT_TRUE(foo(s1, s2));
    s2[1] = 'x';
    ASSERT_FALSE(foo(s1, s2));
}

#if defined(DEBUG) || defined(_DEBUG)
TEST_F(X64AssemblerTest, SSE42StringEqual) {
    __ Reset();
    __ pushq(rbp);
    __ movq(rbp, rsp);
    __ subq(rsp, 16);

    __ movl(rbx, Argv_2); // len
    __ movl(Operand(rbp, -4), Argv_2);
    __ andl(rbx, 0xfffffff0);

    __ movl(rax, 16); // string.1 len
    __ movl(rdx, 16); // string.2 len
    Label cmp_loop;
    __ Bind(&cmp_loop);
    __ cmpl(rbx, 0);
    Label remain;
    __ j(LessEqual, &remain, false);
    __ subl(rbx, 16);
    __ movdqu(xmm0, Operand(Argv_0, rbx, times_1, 0));
    __ pcmpestri(xmm0, Operand(Argv_1, rbx, times_1, 0), PCMP::EqualEach|PCMP::NegativePolarity);
    Label not_equal;
    __ j(Carry, &not_equal, false);
    __ jmp(&cmp_loop, false);

    __ Bind(&remain);
    __ movl(rbx, Operand(rbp, -4)); // len
    __ andl(rbx, 0xf);
    __ movl(rax, rbx); // string.1 len
    __ movl(rdx, rbx); // string.2 len
    
    __ movl(rbx, Operand(rbp, -4));
    __ andl(rbx, 0xfffffff0);
    __ movdqu(xmm0, Operand(Argv_0, rbx, times_1, 0));
    __ pcmpestri(xmm0, Operand(Argv_1, rbx, times_1, 0), PCMP::EqualEach|PCMP::NegativePolarity);
    __ j(Carry, &not_equal, false);
    
    __ movl(rax, 1);
    __ addq(rsp, 16);
    __ popq(rbp);
    __ ret(0);
    
    __ Bind(&not_equal);
    __ xorq(rax, rax);
    __ addq(rsp, 16);
    __ popq(rbp);
    __ ret(0);

    const char *s1 = "012345679";
    const char *s2 = "012345679";
    auto foo = MakeFunction<int (const char *, const char *, int)>();

    ASSERT_TRUE(foo(s1, s2, static_cast<int>(strlen(s1))));

    s2 = "112345679";
    ASSERT_FALSE(foo(s1, s2, static_cast<int>(strlen(s1))));

    s1 = "0123456789abcdef0";
    s2 = "0123456789abcdef0";
    ASSERT_TRUE(foo(s1, s2, static_cast<int>(strlen(s1))));
    
    s1 = "0123456789abcdef0";
    s2 = "0123456789abcdef1";
    ASSERT_FALSE(foo(s1, s2, static_cast<int>(strlen(s1))));
    
    s1 = "0123456789abcdef0123456789abcdef0";
    s2 = "0123456789abcdef0123456789abcdef0";
    ASSERT_TRUE(foo(s1, s2, static_cast<int>(strlen(s1))));
    
    s1 = "0123456789abcdef0123456789abcdef0";
    s2 = "0123456789abcdef0123456789abcdefx";
    ASSERT_FALSE(foo(s1, s2, static_cast<int>(strlen(s1))));
    
    s1 = "0123456789abcdef0123456789abcdef0";
    s2 = "01x3456789abcdef0123456789abcdef0";
    ASSERT_FALSE(foo(s1, s2, static_cast<int>(strlen(s1))));
    
    s1 = "0123456789abcdef0123456789abcdef0";
    s2 = "0123456789abcdef0123x56789abcdef0";
    ASSERT_FALSE(foo(s1, s2, static_cast<int>(strlen(s1))));
}
#endif // #if defined(DEBUG) || defined(_DEBUG)

TEST_F(X64AssemblerTest, MemoryBarrier) {
    __ Reset();
    __ pushq(rbp);
    __ movq(rbp, rsp);

    __ movl(Operand(Argv_0, 0), Argv_1);
    __ sfence();

    __ popq(rbp);
    __ ret(0);
    
    auto foo = MakeFunction<void (int *, int)>();
    int n = 0;
    foo(&n, 996);
    ASSERT_EQ(996, n);
}


} // namespace x64
    
} // namespace mai
