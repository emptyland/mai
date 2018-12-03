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
                                  const ColumnFamilyImpl *cfd,
                                  uint64_t file_number, uint64_t file_size) {
    base::intrusive_ptr<Entry> entry;
    Error rs = EnsureTableCached(cfd, file_number, file_size, &entry);
    if (!rs) {
        return Iterator::AsError(rs);
    }
    Iterator *iter = entry->table->NewIterator(read_opts, cfd->ikcmp());
    if (iter->error().ok()) {
        entry->AddRef();
        iter->RegisterCleanup(&EntryCleanup, entry.get());
    }
    return iter;
}
    
Error TableCache::Get(const ReadOptions &read_opts, const ColumnFamilyImpl *cfd,
                      uint64_t file_number, std::string_view key, core::Tag *tag,
                      std::string *value) {
    base::intrusive_ptr<Entry> entry;
    Error rs = EnsureTableCached(cfd, file_number, 0, &entry);
    if (!rs) {
        return rs;
    }
    std::string_view result;
    std::string scatch;
    rs = entry->table->Get(read_opts, cfd->ikcmp(), key, tag, &result, &scatch);
    if (!rs) {
        return rs;
    }
    value->assign(result.data(), result.size());
    return Error::OK();
}
    
Error
TableCache::GetTableProperties(const ColumnFamilyImpl *cfd, uint64_t file_number,
                               base::intrusive_ptr<table::TablePropsBoundle> *props) {
    base::intrusive_ptr<Entry> entry;
    Error rs = EnsureTableCached(cfd, file_number, 0, &entry);
    if (!rs) {
        return rs;
    }
    *props = entry->table->GetTableProperties();
    return Error::OK();
}

Error TableCache::GetKeyFilter(const ColumnFamilyImpl *cfd, uint64_t file_number,
                               base::intrusive_ptr<core::KeyFilter> *filter) {
    base::intrusive_ptr<Entry> entry;
    Error rs = EnsureTableCached(cfd, file_number, 0, &entry);
    if (!rs) {
        return rs;
    }
    *filter = entry->table->GetKeyFilter();
    return Error::OK();
}
    
Error TableCache::EnsureTableCached(const ColumnFamilyImpl *cfd,
                                    uint64_t file_number, uint64_t file_size,
                                    base::intrusive_ptr<Entry> *result) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto iter = cached_.find(file_number);
    if (iter != cached_.end()) {
        *result = iter->second;
        return Error::OK();
    }
    
    base::intrusive_ptr<Entry> entry(new Entry);
    entry->file_name = cfd->GetTableFileName(file_number);
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
    mutex_.unlock();
    rs = factory_->NewTableReader(cfd->options().use_unordered_table ?
                                  "s1t" : "sst",
                                  cfd->ikcmp(),
                                  entry->file.get(), file_size, true,
                                  &entry->table);
    mutex_.lock();
    if (!rs) {
        return rs;
    }
    
    entry->cfid = cfd->id();
    *result = entry;
    cached_[file_number] = entry;
    return Error::OK();
}
    
/*static*/ void TableCache::EntryCleanup(void *arg1, void *arg2) {
    static_cast<Entry *>(arg1)->ReleaseRef();
}
    
} // namespace db
    
} // namespace mai
