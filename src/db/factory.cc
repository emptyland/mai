#include "db/factory.h"
#include "db/compaction-impl.h"
#include "table/xhash-table-reader.h"
#include "table/xhash-table-builder.h"
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
    NewMemoryTable(const core::InternalKeyComparator *ikcmp,
                   Allocator *allocator, bool unordered,
                   size_t initial_slots) override {
        if (unordered) {
            return new core::UnorderedMemoryTable(ikcmp,
                                                  static_cast<int>(initial_slots),
                                                  allocator);
        } else {
            return new core::OrderedMemoryTable(ikcmp, allocator);
        }
    }
    
    virtual Error
    NewTableReader(const std::string &name,
                   const core::InternalKeyComparator *ikcmp,
                   RandomAccessFile *file, uint64_t file_size,
                   bool checksum_verify,
                   std::unique_ptr<table::TableReader> *result) override {
        if (name.compare("xmt") == 0) {
            table::XhashTableReader *reader =
                new table::XhashTableReader(file, file_size);
            Error rs = reader->Prepare();
            if (!rs) {
                delete reader;
                return rs;
            }
            result->reset(reader);
        } else if (name.compare("s1t") == 0) {
            table::S1TableReader *reader =
                new table::S1TableReader(file, file_size, checksum_verify);
            Error rs = reader->Prepare();
            if (!rs) {
                delete reader;
                return rs;
            }
            result->reset(reader);
        } else if (name.compare("sst") == 0) {
            table::SstTableReader *reader =
                new table::SstTableReader(file, file_size, checksum_verify);
            Error rs = reader->Prepare();
            if (!rs) {
                delete reader;
                return rs;
            }
            result->reset(reader);
        } else {
            return MAI_CORRUPTION(Slice::Sprintf("Unknown table name: %s",
                                                 name.c_str()));
        }
        return Error::OK();
    }

    virtual table::TableBuilder *
    NewTableBuilder(const std::string &name,
                    const core::InternalKeyComparator *ikcmp,
                    WritableFile *file, uint64_t block_size, int n_restart,
                    size_t max_hash_slots) override {
        if (name.compare("xmt") == 0) {
            return new table::XhashTableBuilder(ikcmp, file, max_hash_slots,
                                                static_cast<uint32_t>(block_size));
        } else if (name.compare("s1t") == 0) {
            return new table::S1TableBuilder(ikcmp, file, max_hash_slots,
                                             static_cast<uint32_t>(block_size));
        } else if (name.compare("sst") == 0) {
            return new table::SstTableBuilder(ikcmp, file, block_size, n_restart);
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
