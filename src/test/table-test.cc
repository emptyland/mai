#include "test/table-test.h"
#include "table/table.h"

namespace mai {
    
namespace test {
    
/*static*/ std::atomic<uint64_t> TableTest::mock_file_number_(1);
    
void TableTest::BuildTable(const std::vector<std::string> &kvs,
                           const std::string &file_name,
                           TableBuilderFactory factory) {
    std::unique_ptr<WritableFile> file;
    auto rs = env_->NewWritableFile(file_name, false, &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    file->Truncate(0);
    std::unique_ptr<table::TableBuilder> builder(factory(&ikcmp_, file.get()));
    ASSERT_GE(kvs.size(), 3);
    for (int i = 0; i < kvs.size(); i += 3) {
        auto k = kvs[i];
        auto v = kvs[i + 1];
        auto s = ::atoi(kvs[i + 2].c_str());
        
        Add(builder.get(), k, v, s, core::Tag::kFlagValue);
    }
    
    rs = builder->Finish();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}

void TableTest::NewReader(const std::string &file_name,
                          std::unique_ptr<RandomAccessFile> *file,
                          std::unique_ptr<table::TableReader> *reader,
                          TableReaderFactory factory) {
    auto rs = env_->NewRandomAccessFile(file_name, file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size;
    rs = (*file)->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto fn = mock_file_number_.fetch_add(1);
    reader->reset(factory((*file).get(), fn, file_size, &block_cache_));
    ASSERT_TRUE(rs.ok()) << rs.ToString();
}
    
void TableTest::ReadProperties(const char *file_name, uint32_t *magic_number,
                               std::unique_ptr<table::TableProperties> *props) {
    using base::Slice;
    
    std::unique_ptr<RandomAccessFile> file;
    auto rs = env_->NewRandomAccessFile(file_name, &file);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    uint64_t file_size = 0;
    rs = file->GetFileSize(&file_size);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string scratch;
    std::string_view result;
    rs = file->Read(file_size - 4, 4, &result, &scratch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    *magic_number = Slice::SetU32(result);
    
    rs = file->Read(file_size - 12, 8, &result, &scratch);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    auto props_pos = Slice::SetU64(result);
    
    std::unique_ptr<table::TableProperties> rv(new table::TableProperties{});
    rs = table::Table::ReadProperties(file.get(), props_pos + 4,
                                      file_size - 12 - props_pos - 4,
                                      rv.get());
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    props->reset(rv.release());
}
    
} // namespace test
    
} // namespace mai
