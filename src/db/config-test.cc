#include "db/config.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace db {
    
TEST(ConfigTest, ComputeNumSlots) {
    ASSERT_EQ(3449, Config::ComputeNumSlots(0, 1024 + 1, 3.0, 17));
    ASSERT_EQ(127, Config::ComputeNumSlots(0, 1024 + 1, 0.1, 17));
    ASSERT_EQ(103, Config::ComputeNumSlots(1, 1024 + 1, 0.1, 17));
    ASSERT_EQ(89, Config::ComputeNumSlots(2, 1024 + 1, 0.1, 17));
    ASSERT_EQ(79, Config::ComputeNumSlots(3, 1024 + 1, 0.1, 17));
}
    
TEST(ConfigTest, ComputeNumSlotsV2) {
    EXPECT_EQ(68461, Config::ComputeNumSlots(0, 70000, Config::kLimitMinNumberSlots));
    EXPECT_EQ(53621, Config::ComputeNumSlots(1, 70000, Config::kLimitMinNumberSlots));
    EXPECT_EQ(38783, Config::ComputeNumSlots(2, 70000, Config::kLimitMinNumberSlots));
    EXPECT_EQ(23941, Config::ComputeNumSlots(3, 70000, Config::kLimitMinNumberSlots));
}
    
} // namespace db
    
} // namespace mai
