#ifndef MAI_TEST_NYAA_TEST_H_
#define MAI_TEST_NYAA_TEST_H_

#include "mai-lang/nyaa.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace test {

class NyaaTest : public ::testing::Test {
public:
    NyaaTest() : isolate_(isolate_ = nyaa::Isolate::New()) {}
    
    ~NyaaTest() override { isolate_->Dispose(); }
    
    void SetUp() override {
        isolate_->Enter();
    }
    
    void TearDown() override {
        isolate_->Exit();
    }
    
    nyaa::Isolate *isolate_;
};

} // namespace test
    
} // namespace mai

#endif // MAI_TEST_NYAA_TEST_H_
