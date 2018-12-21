#include "db/compaction.h"
#include "db/compaction-impl.h"
#include "db/factory.h"
#include "db/table-cache.h"
#include "db/version.h"
#include "db/column-family.h"
#include "table/sst-table-reader.h"
#include "table/sst-table-builder.h"
#include "table/block-cache.h"
#include "mai/iterator.h"
#include "test/table-test.h"
#include "gtest/gtest.h"


namespace mai {

namespace db {
    
class CompactionImplTest : public test::TableTest {
public:
    CompactionImplTest()
        : abs_db_path_(env_->GetAbsolutePath("tests/99-compaction-tmp"))
        , factory_(Factory::NewDefault())
        , table_cache_(new TableCache(abs_db_path_, options_, &block_cache_,
                                      factory_.get()))
        , versions_(new VersionSet(abs_db_path_, Options{}, table_cache_.get())) {

        versions_->column_families()->NewColumnFamily(ColumnFamilyOptions{},
                                                      kDefaultColumnFamilyName,
                                                      0, versions_.get());
        cfd_ = versions_->column_families()->GetDefault();
    }
    
    void SetUp() override {
        Error rs = env_->MakeDirectory(abs_db_path_, true);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        rs = cfd_->Install(factory_.get());
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    void TearDown() override {
        env_->DeleteFile(abs_db_path_, true);
    }
    
    void AppendFile(uint64_t fid, int level) {
        base::intrusive_ptr<table::TablePropsBoundle> boundle;
        
        Error rs = table_cache_->GetTableProperties(cfd_, fid, &boundle);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        //boundle->data().smallest_key
        auto name = cfd_->GetTableFileName(fid);
        
        auto fmd = new FileMetaData(fid);
        fmd->smallest_key = boundle->data().smallest_key;
        fmd->largest_key  = boundle->data().largest_key;
        fmd->ctime        = env_->CurrentTimeMicros();
        
        rs = env_->GetFileSize(name, &fmd->size);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        VersionPatch patch;
        patch.CreaetFile(cfd_->id(), level, fmd);
        
        rs = versions_->LogAndApply(ColumnFamilyOptions{}, &patch, nullptr);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    void Compact(const std::vector<uint64_t> &inputs, int target_level,
                 core::SequenceNumber smallest_snapshot,
                 uint64_t *target_file_number, CompactionResult *result) {
        std::unique_ptr<Compaction>
        job(factory_->NewCompaction(abs_db_path_, &ikcmp_,
                                    table_cache_.get(), cfd_));
        ASSERT_TRUE(job.get() != nullptr);
        
        job->set_target_level(target_level);
        job->set_input_version(cfd_->current());
        job->set_smallest_snapshot(smallest_snapshot);
        job->set_target_file_number(versions_->GenerateFileNumber());
        if (target_file_number) {
            *target_file_number = job->target_file_number();
        }
        
        for (auto fid : inputs) {
            Iterator *iter = table_cache_->NewIterator(ReadOptions{}, cfd_, fid, 0);
            job->AddInput(iter);
        }
        
        std::unique_ptr<WritableFile> file;
        Error rs = env_->NewWritableFile(cfd_->GetTableFileName(job->target_file_number()), true, &file);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        std::unique_ptr<table::TableBuilder> builder(default_tb_factory_(&ikcmp_, file.get()));
        rs = job->Run(builder.get(), result);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
    }
    
    TableBuilderFactory default_tb_factory_
        = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
        return new table::SstTableBuilder(ikcmp, file, 512, 3);
    };
    
    TableBuilderFactory k13_restart_tb_factory_
        = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
        return new table::SstTableBuilder(ikcmp, file, 512, 13);
    };
    
    TableReaderFactory default_tr_factory_
        = [](RandomAccessFile *file, uint64_t file_number, uint64_t file_size,
             table::BlockCache *cache) {
        return new table::SstTableReader(file, file_number, file_size, true, cache);
    };
    
    std::string abs_db_path_;
    Options options_;
    std::unique_ptr<Factory> factory_;
    std::unique_ptr<TableCache> table_cache_;
    std::unique_ptr<VersionSet> versions_;
    ColumnFamilyImpl *cfd_;
};
    
TEST_F(CompactionImplTest, Sanity) {
    auto fid = versions_->GenerateFileNumber();
    auto name = cfd_->GetTableFileName(fid);
    BuildTable({
        "k1", "v1", "1",
        "k2", "v2", "2",
        "k3", "v3", "3",
        "k4", "v4", "4",
        "k5", "v6", "6",
        "k5", "v5", "5",
    }, name, default_tb_factory_);
    AppendFile(fid, 0);

    uint64_t target_fid;
    CompactionResult result;
    Compact({fid}, 1, 7, &target_fid, &result);
    
    std::unique_ptr<RandomAccessFile> file;
    std::unique_ptr<table::TableReader> reader;
    NewReader(cfd_->GetTableFileName(target_fid), &file, &reader, default_tr_factory_);
    Error rs = static_cast<table::SstTableReader *>(reader.get())->Prepare();
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    std::string value;
    rs = Get(reader.get(), "k5", 7, &value, nullptr);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_EQ("v6", value);
    
    rs = Get(reader.get(), "k5", 5, &value, nullptr);
    ASSERT_TRUE(rs.IsNotFound());
}
    
} // namespace db
    
} // namespace mai
