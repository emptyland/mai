#include "mai/error.h"
#include "gtest/gtest.h"

namespace mai {
    
TEST(ErrorTest, Sanity) {
    Error err = Error::OK();
    ASSERT_TRUE(err.ok());
}

TEST(ErrorTest, NotFound) {
    Error err = MAI_NOT_FOUND("Key has delete flag.");
    ASSERT_TRUE(err.IsNotFound());
}
    
TEST(ErrorTest, ToString) {
    Error err = MAI_EOF("EOF");
    ASSERT_TRUE(err.IsEof());
    ASSERT_EQ("[core/error-test.cc:17] EOF", err.ToString());
}
    
} // namespace mai
