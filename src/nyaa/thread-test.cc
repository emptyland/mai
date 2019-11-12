#include "nyaa/thread.h"
#include "test/nyaa-test.h"
#include "nyaa/nyaa-core.h"

namespace mai {

namespace nyaa {

class NyaaThreadTest : public test::NyaaTest {
public:
    void SetUp() override {
        NyaaTest::SetUp();
        vm_ = new Nyaa(NyaaOptions{}, isolate_);
        N_ = vm_->core();
    }
    
    void TearDown() override {
        N_ = nullptr;
        delete vm_;
        NyaaTest::TearDown();
    }

    Nyaa *vm_ = nullptr;
    NyaaCore *N_ = nullptr;
};

TEST_F(NyaaThreadTest, Sanity) {
    ASSERT_NE(nullptr, N_->current_thread());
    ASSERT_NE(nullptr, N_->current_thread()->core());
    
    HandleScope handle_scope(vm_);
    Handle<NyThread> thrd(N_->current_thread());
    
    ASSERT_EQ(nullptr, thrd->next());
    ASSERT_EQ(nullptr, thrd->prev());
    ASSERT_EQ(N_, thrd->owns());
}

}

}
