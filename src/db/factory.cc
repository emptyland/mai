#include "db/factory.h"
#include "db/compaction-impl.h"
#include "table/xhash-table-reader.h"
#include "table/xhash-table-builder.h"
#include "table/sst-table-reader.h"
#include "table/sst-table-builder.h"
#include "core/unordered-memory-table.h"
#include "core/ordered-memory-table.h"

namespace mai {
    
namespace db {

class FactoryImpl final : public Factory {
public:
    FactoryImpl() {}
    virtual ~FactoryImpl() {}
    
    virtual core::MemoryTable *
    NewMemoryTable(const core::InternalKeyComparator *ikcmp, bool unordered,
                   size_t initial_slots) override {
        if (unordered) {
            return new core::UnorderedMemoryTable(ikcmp,
                                                  static_cast<int>(initial_slots));
        } else {
            return new core::OrderedMemoryTable(ikcmp);
        }
    }
    
    virtual Error
    NewTableReader(const core::InternalKeyComparator *ikcmp, bool unordered,
                   RandomAccessFile *file, uint64_t file_size,
                   bool checksum_verify,
                   std::unique_ptr<table::TableReader> *result) override {
        if (unordered) {
            table::XhashTableReader *reader =
                new table::XhashTableReader(file, file_size);
            Error rs = reader->Prepare();
            if (!rs) {
                delete reader;
                return rs;
            }
            result->reset(reader);
        } else {
            table::SstTableReader *reader =
                new table::SstTableReader(file, file_size, checksum_verify);
            Error rs = reader->Prepare();
            if (!rs) {
                delete reader;
                return rs;
            }
            result->reset(reader);
        }
        return Error::OK();
    }

    virtual table::TableBuilder *
    NewTableBuilder(const core::InternalKeyComparator *ikcmp, bool unordered,
                    WritableFile *file, uint64_t block_size, int n_restart,
                    size_t max_hash_slots) override {
        if (unordered) {
            return new table::XhashTableBuilder(ikcmp, file, max_hash_slots,
                                                static_cast<uint32_t>(block_size));
            
        } else {
            return new table::SstTableBuilder(ikcmp, file, block_size, n_restart);
        }
    }
    
    virtual Compaction *
    NewCompaction(const std::string abs_db_path,
                  const core::InternalKeyComparator *ikcmp,
                  TableCache *table_cache, ColumnFamilyImpl *cfd) override {
        return new CompactionImpl(abs_db_path, ikcmp, table_cache, cfd);
    }
    
}; // class FactoryImpl
    
/*static*/ Factory *Factory::NewDefault() {
    return new FactoryImpl();
}
    
} // namespace db
    
} // namespace mai
