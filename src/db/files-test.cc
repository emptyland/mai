#include "db/files.h"
#include "gtest/gtest.h"

namespace mai {

namespace db {

TEST(FilesTest, Sanity) {
    EXPECT_EQ("demo/MANIFEST-1", Files::ManifestFileName("demo", 1));
    EXPECT_EQ("demo/1.log", Files::LogFileName("demo", 1));
    
    EXPECT_EQ("demo/default/1.sst", Files::TableFileName("demo", "default",
                                                         Files::kSST_Table, 1));
}
    
TEST(FilesTest, ParseName) {
    Files::Kind kind;
    uint64_t number;
    std::tie(kind, number) = Files::ParseName("1.sst");
    EXPECT_EQ(Files::kSST_Table, kind);
    EXPECT_EQ(1, number);
    
    std::tie(kind, number) = Files::ParseName("1.xmt");
    EXPECT_EQ(Files::kXMT_Table, kind);
    EXPECT_EQ(1, number);
    
    std::tie(kind, number) = Files::ParseName("MANIFEST-200");
    EXPECT_EQ(Files::kManifest, kind);
    EXPECT_EQ(200, number);
}
    
} // namespace db
    
} // namespace mai
