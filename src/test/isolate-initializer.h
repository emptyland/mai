#include "lang/isolate-inl.h"
#include "gtest/gtest.h"

namespace mai {

namespace test {
    
class IsolateInitializer : public ::testing::Test {
public:
    IsolateInitializer() : isolate_(lang::Isolate::New(lang::Options{})) {}
    
    void SetUp() override {
        if (auto err = isolate_->Initialize(); err.fail()) {
            FAIL() << err.ToString();
        }
        isolate_->Enter();
    }
    
    void TearDown() override {
        isolate_->Exit();
        isolate_->Dispose();
    }
    
    lang::Isolate *isolate_;
}; // class IsolateInitializer

} // namespace test
    
} // namespace mai
