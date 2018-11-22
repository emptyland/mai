#include "mai/env.h"
#include "gtest/gtest.h"

namespace mai {
    
TEST(WritableFileTest, Sanity) {
    std::unique_ptr<WritableFile> file;
    auto rv = Env::Default()->NewWritableFile("tests/00-writable-file.tmp",
                                              false, &file);
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    
    rv = file->Append("Hello, World!\n");
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    
    rv = file->PositionedAppend("1024", 0);
    ASSERT_TRUE(rv.ok()) << rv.ToString();

    ASSERT_TRUE(rv.ok()) << rv.ToString();
}

TEST(RandomAccessFileTest, Sanity) {
    std::unique_ptr<WritableFile> wf;
    auto rv = Env::Default()->NewWritableFile("tests/01-random-access-file.tmp",
                                              false, &wf);
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    
    rv = wf->Append("Hello, World!\n");
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    wf.reset();
    
    std::unique_ptr<RandomAccessFile> rf;
    rv = Env::Default()->NewRandomAccessFile("tests/01-random-access-file.tmp",
                                             &rf);
    ASSERT_TRUE(rv.ok()) << rv.ToString();
    
    std::string_view data;
    rf->Read(0, 5, &data, nullptr);
    ASSERT_EQ("Hello", data);
}

} // namespace mai
