#include "db/factory.h"
#include "db/compaction-impl.h"
#include "table/sst-table-reader.h"
#include "table/sst-table-builder.h"
#include "table/s1-table-reader.h"
#include "table/s1-table-builder.h"
#include "core/unordered-memory-table.h"
#include "core/ordered-memory-table.h"
#include "base/slice.h"

namespace mai {
    
namespace db {
    
using ::mai::base::Slice;

class FactoryImpl final : public Factory {
public:
    FactoryImpl() {}
    virtual ~FactoryImpl() {}
    
    virtual core::MemoryTable *
    NewMemoryTable(const core::InternalKeyComparator *ikcmp, bool unordered,
                   size_t initial_slots) override {
        if (unordered) {
            return new core::UnorderedMemoryTable(ikcmp, static_cast<int>(initial_slots));
        } else {
            return new core::OrderedMemoryTable(ikcmp);
        }
    }
    
    virtual Error
    NewTableReader(const std::string &name,
                   const core::InternalKeyComparator *ikcmp,
                   RandomAccessFile *file,
                   uint64_t file_number,
                   uint64_t file_size,
                   bool checksum_verify,
                   table::BlockCache *cache,
                   std::unique_ptr<table::TableReader> *result) override {
        if (name.compare("s1t") == 0) {
            table::S1TableReader *reader =
                new table::S1TableReader(file, file_number, file_size,
                                         checksum_verify, cache);
            Error rs = reader->Prepare();
            if (!rs) {
                delete reader;
                return rs;
            }
            result->reset(reader);
        } else if (name.compare("sst") == 0) {
            table::SstTableReader *reader =
                new table::SstTableReader(file, file_number, file_size,
                                          checksum_verify, cache);
            Error rs = reader->Prepare();
            if (!rs) {
                delete reader;
                return rs;
            }
            result->reset(reader);
        } else {
            return MAI_CORRUPTION(base::Sprintf("Unknown table name: %s",
                                                 name.c_str()));
        }
        return Error::OK();
    }

    virtual table::TableBuilder *
    NewTableBuilder(const std::string &name,
                    const core::InternalKeyComparator *ikcmp,
                    WritableFile *file, uint64_t block_size, int n_restart,
                    size_t max_hash_slots,
                    size_t approximated_n_entries) override {
        if (name.compare("s1t") == 0) {
            return new table::S1TableBuilder(ikcmp, file, max_hash_slots,
                                             static_cast<uint32_t>(block_size),
                                             approximated_n_entries);
        } else if (name.compare("sst") == 0) {
            return new table::SstTableBuilder(ikcmp, file, block_size, n_restart,
                                              approximated_n_entries);
        }
        return nullptr;
    }
    
    virtual Compaction *
    NewCompaction(const std::string &abs_db_path,
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
