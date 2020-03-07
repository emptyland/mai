#pragma once
#ifndef MAI_TEST_ISOLATE_INITIALIZER_H_
#define MAI_TEST_ISOLATE_INITIALIZER_H_

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
        if (auto err = isolate_->LoadBaseLibraries(); err.fail()) {
            FAIL() << err.ToString();
        }
    }
    
    void TearDown() override {
        isolate_->Exit();
        isolate_->Dispose();
    }
    
    lang::Isolate *isolate_;
}; // class IsolateInitializer

} // namespace test
    
} // namespace mai

#endif // MAI_TEST_ISOLATE_INITIALIZER_H_
