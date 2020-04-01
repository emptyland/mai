#include "lang/marking-sweep.h"
#include "lang/heap.h"
#include "test/isolate-initializer.h"
#include "base/slice.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class MarkingSweepTest : public test::IsolateInitializer {
public:
    MarkingSweepTest() {}
    
    void SetUp() override {
        IsolateInitializer::SetUp();
    }
    void TearDown() override {
        IsolateInitializer::TearDown();
    }
}; // class MarkingSweepTest

TEST_F(MarkingSweepTest, Sanity) {
    HandleScope handle_scope(HandleScope::INITIALIZER);
    MarkingSweep marking_sweep(isolate_, isolate_->heap());
    base::StdFilePrinter printer(stdout);
    marking_sweep.Run(&printer);
}

} // namespace lang

} // namespace mai
