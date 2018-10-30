#include "error.h"
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
    
} // namespace mai
