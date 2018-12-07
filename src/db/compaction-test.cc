#include "db/compaction.h"
#include "db/compaction-impl.h"
#include "db/factory.h"
#include "table/sst-table-reader.h"
#include "table/sst-table-builder.h"
#include "test/table-test.h"
#include "gtest/gtest.h"


namespace mai {

namespace db {
    
class CompactionImplTest : public test::TableTest {
    
    TableBuilderFactory default_tb_factory_
        = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
        return new table::SstTableBuilder(ikcmp, file, 512, 3);
    };
    
    TableBuilderFactory k13_restart_tb_factory_
        = [](const core::InternalKeyComparator *ikcmp, WritableFile *file) {
        return new table::SstTableBuilder(ikcmp, file, 512, 13);
    };
    
    TableReaderFactory default_tr_factory_
        = [](RandomAccessFile *file, uint64_t file_size) {
        return new table::SstTableReader(file, file_size, true);
    };
    
    std::string abs_db_path_;
    Options options_;
    std::unique_ptr<Factory> factory_;
    std::unique_ptr<TableCache> table_cache_;
    std::unique_ptr<VersionSet> versions_;
};
    

    
} // namespace db
    
} // namespace mai
