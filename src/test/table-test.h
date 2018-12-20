#ifndef MAI_TEST_TABLE_TEST_H_
#define MAI_TEST_TABLE_TEST_H_

#include "table/table-builder.h"
#include "table/table-reader.h"
#include "table/block-cache.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "base/hash.h"
#include "mai/comparator.h"
#include "mai/env.h"
#include "mai/options.h"
#include "gtest/gtest.h"
#include <functional>

namespace mai {

namespace test {
    
class TableTest : public ::testing::Test {
public:
    using TableBuilderFactory =
        std::function<table::TableBuilder *(const core::InternalKeyComparator *,
                                            WritableFile *file)>;

    using TableReaderFactory =
        std::function<table::TableReader *(RandomAccessFile *,
                                           uint64_t,
                                           table::BlockCache *)>;
    
    TableTest()
        : ikcmp_(Comparator::Bytewise())
        , block_cache_(env_->GetLowLevelAllocator(), 1000) {}
    
    void Add(table::TableBuilder *builder,
             std::string_view key,
             std::string_view value,
             core::SequenceNumber version,
             uint8_t flag) {
        base::ScopedMemory scope;
        auto ikey = core::KeyBoundle::MakeKey(key, version, flag);
        builder->Add(ikey, value);
        ASSERT_TRUE(builder->error().ok()) << builder->error().ToString();
    }
    
    Error Get(table::TableReader *reader,
              std::string_view key,
              core::SequenceNumber version,
              std::string *value,
              core::Tag *tag) {
        base::ScopedMemory scope;
        auto ikey = core::KeyBoundle::New(key, version);
        
        std::string_view result;
        std::string scratch;
        auto rs = reader->Get(ReadOptions{}, &ikcmp_, ikey->key(), tag,
                              &result, &scratch);
        *value = result;
        return rs;
    }
    
    void BuildTable(const std::vector<std::string> &kvs,
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
    
    void NewReader(const std::string &file_name,
                   std::unique_ptr<RandomAccessFile> *file,
                   std::unique_ptr<table::TableReader> *reader,
                   TableReaderFactory factory) {
        auto rs = env_->NewRandomAccessFile(file_name, file);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        uint64_t file_size;
        rs = (*file)->GetFileSize(&file_size);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        reader->reset(factory((*file).get(), file_size, &block_cache_));
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    void ReadProperties(const char *file_name,
                        uint32_t *magic_number,
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
    
    Env *env_ = Env::Default();
    core::InternalKeyComparator ikcmp_;
    table::BlockCache block_cache_;
}; // class TableTest
    
} // namespace test
    
} // namespace mai


#endif // MAI_TEST_TABLE_TEST_H_
