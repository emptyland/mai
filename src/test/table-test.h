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
                    TableBuilderFactory factory);
    
    void NewReader(const std::string &file_name,
                   std::unique_ptr<RandomAccessFile> *file,
                   std::unique_ptr<table::TableReader> *reader,
                   TableReaderFactory factory);
    
    void ReadProperties(const char *file_name, uint32_t *magic_number,
                        std::unique_ptr<table::TableProperties> *props);
    
    Env *env_ = Env::Default();
    core::InternalKeyComparator ikcmp_;
    table::BlockCache block_cache_;
    
    static std::atomic<uint64_t> mock_file_number_;
}; // class TableTest
    
} // namespace test
    
} // namespace mai


#endif // MAI_TEST_TABLE_TEST_H_
