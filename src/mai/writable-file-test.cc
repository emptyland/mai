#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
TEST(WritableFileTest, Sanity) {
    std::unique_ptr<WritableFile> file;
    auto rv = Env::Default()->NewWritableFile("tests/00-writable-file.tmp", &file);
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    
    rv = file->Append("Hello, World!\n");
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    
    rv = file->PositionedAppend("1024", 0);
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    
    rv = file->Close();
    ASSERT_TRUE(rv.ok()) << rv.ToString();
}
    
} // namespace mai
