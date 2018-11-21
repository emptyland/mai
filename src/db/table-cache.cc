#include "db/table-cache.h"
#include "db/column-family.h"
#include "db/factory.h"
#include "db/version.h"
#include "table/table.h"
#include "core/key-filter.h"
#include "mai/iterator.h"
#include "mai/options.h"

namespace mai {
    
namespace db {
    
TableCache::~TableCache() {}

Iterator *TableCache::NewIterator(const ReadOptions &read_opts,
                                  const ColumnFamilyImpl *cf,
                                  uint64_t file_number, uint64_t file_size) {
    base::Handle<Entry> entry;
    Error rs = EnsureTableCached(cf, file_number, file_size, &entry);
    if (!rs) {
        return Iterator::AsError(rs);
    }
    Iterator *iter = entry->table->NewIterator(read_opts, cf->ikcmp());
    if (iter->error().ok()) {
        entry->AddRef();
        iter->RegisterCleanup(&EntryCleanup, entry.get());
    }
    return iter;
}

Error TableCache::GetFileMetadata(const ColumnFamilyImpl *cf,
                                  uint64_t file_number, FileMetadata *fmd) {
    base::Handle<Entry> entry;
    Error rs = EnsureTableCached(cf, file_number, 0, &entry);
    if (!rs) {
        return rs;
    }
    fmd->number = file_number;
    rs = entry->file->GetFileSize(&fmd->size);
    if (!rs) {
        return rs;
    }
    fmd->largest_key  = entry->table->GetTableProperties()->largest_key;
    fmd->smallest_key = entry->table->GetTableProperties()->smallest_key;
    return Error::OK();
}
    
Error
TableCache::GetTableProperties(const ColumnFamilyImpl *cf, uint64_t file_number,
                               std::shared_ptr<table::TableProperties> *props) {
    base::Handle<Entry> entry;
    Error rs = EnsureTableCached(cf, file_number, 0, &entry);
    if (!rs) {
        return rs;
    }
    *props = entry->table->GetTableProperties();
    return Error::OK();
}

Error TableCache::GetKeyFilter(const ColumnFamilyImpl *cf, uint64_t file_number,
                               std::shared_ptr<core::KeyFilter> *filter) {
    base::Handle<Entry> entry;
    Error rs = EnsureTableCached(cf, file_number, 0, &entry);
    if (!rs) {
        return rs;
    }
    *filter = entry->table->GetKeyFilter();
    return Error::OK();
}
    
Error TableCache::EnsureTableCached(const ColumnFamilyImpl *cf,
                                    uint64_t file_number, uint64_t file_size,
                                    base::Handle<Entry> *result) {
    auto iter = cached_.find(file_number);
    if (iter != cached_.end()) {
        *result = iter->second;
        return Error::OK();
    }
    
    base::Handle<Entry> entry(new Entry);
    entry->file_name = cf->GetTableFileName(file_number);
    Error rs = env_->NewRandomAccessFile(entry->file_name, &entry->file,
                                         allow_mmap_reads_);
    if (!rs) {
        return rs;
    }
    if (file_size == 0) {
        rs = entry->file->GetFileSize(&file_size);
        if (!rs) {
            return rs;
        }
    }
    rs = factory_->NewTableReader(cf->options().use_unordered_table,
                                  entry->file.get(), file_size, true,
                                  &base::Hash::Js, &entry->table);
    if (!rs) {
        return rs;
    }
    
    entry->cfid = cf->id();
    *result = entry;
    cached_[file_number] = entry;
    return Error::OK();
}
    
/*static*/ void TableCache::EntryCleanup(void *arg1, void *arg2) {
    static_cast<Entry *>(arg1)->ReleaseRef();
}
    
} // namespace db
    
} // namespace mai
