#include "lang/marking-compact.h"
#include "lang/heap.h"
#include "test/isolate-initializer.h"
#include "base/slice.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class MarkingCompactTest : public test::IsolateInitializer {
public:
    MarkingCompactTest() {}
    
    void SetUp() override {
        IsolateInitializer::SetUp();
    }
    void TearDown() override {
        IsolateInitializer::TearDown();
    }
}; // class MarkingSweepTest

} // namespace lang

} // namespace mai
