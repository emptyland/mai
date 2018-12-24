#include "db/table-cache.h"
#include "db/column-family.h"
#include "db/factory.h"
#include "db/version.h"
#include "table/table.h"
#include "table/block-cache.h"
#include "core/key-filter.h"
#include "mai/iterator.h"
#include "mai/options.h"
#include <tuple>

namespace mai {
    
namespace db {
    
using Args = std::tuple<const ColumnFamilyImpl *, uint64_t, uint64_t>;
    
/*static*/ void TableCache::EntryDeleter(std::string_view, void *value) {
    static_cast<Entry *>(value)->~Entry();
}
    
/*static*/ Error TableCache::EntryLoader(std::string_view key,
                                         core::LRUHandle **result,
                                         void *arg0, void *arg1) {
    const ColumnFamilyImpl *cfd;
    uint64_t file_number;
    uint64_t file_size;
    std::tie(cfd, file_number, file_size) = *static_cast<Args *>(arg1);
    
    core::LRUHandle *handle = core::LRUHandle::New(key, sizeof(Entry));
    if (!handle) {
        return MAI_CORRUPTION("Out of memory!");
    }
    
    TableCache *self = static_cast<TableCache *>(DCHECK_NOTNULL(arg0));
    Entry *entry = new (handle->value) Entry;
    Error rs = self->LoadTable(cfd, file_number, file_size, entry);
    if (!rs) {
        entry->~Entry();
        core::LRUHandle::Free(handle);
        return rs;
    }
    *result = handle;
    return Error::OK();
}
    
static void HandleCleanup(void *arg1, void *arg2) {
    static_cast<core::LRUHandle *>(arg1)->ReleaseRef();
}
    
TableCache::TableCache(const std::string &abs_db_path, const Options &opts,
                       Factory *factory)
    : abs_db_path_(abs_db_path)
    , env_(DCHECK_NOTNULL(opts.env))
    , block_cache_(new table::BlockCache(opts.env->GetLowLevelAllocator(),
                                         opts.block_cache_capacity))
    , factory_(DCHECK_NOTNULL(factory))
    , allow_mmap_reads_(opts.allow_mmap_reads)
    , cache_(env_->GetLowLevelAllocator(), opts.max_open_files) {}
    
TableCache::~TableCache() {}

Iterator *TableCache::NewIterator(const ReadOptions &read_opts,
                                  const ColumnFamilyImpl *cfd,
                                  uint64_t file_number, uint64_t file_size) {
    base::intrusive_ptr<core::LRUHandle> handle;
    Error rs = GetOrLoadTable(cfd, file_number, file_size, &handle);
    if (!rs) {
        return Iterator::AsError(rs);
    }
    
    Iterator *iter = GetEntry(handle.get())->table->NewIterator(read_opts,
                                                                cfd->ikcmp());
    if (iter->error().ok()) {
        handle->AddRef();
        iter->RegisterCleanup(&HandleCleanup, handle.get());
    }
    return iter;
}
    
Error TableCache::Get(const ReadOptions &read_opts, const ColumnFamilyImpl *cfd,
                      uint64_t file_number, std::string_view key, core::Tag *tag,
                      std::string *value) {
    base::intrusive_ptr<core::LRUHandle> handle;
    Error rs = GetOrLoadTable(cfd, file_number, 0, &handle);
    if (!rs) {
        return rs;
    }
    std::string_view result;
    std::string scatch;
    rs = GetEntry(handle.get())->table->Get(read_opts, cfd->ikcmp(), key, tag,
                                            &result, &scatch);
    if (!rs) {
        return rs;
    }
    value->assign(result.data(), result.size());
    return Error::OK();
}
    
Error
TableCache::GetTableProperties(const ColumnFamilyImpl *cfd, uint64_t file_number,
                               base::intrusive_ptr<table::TablePropsBoundle> *props) {
    base::intrusive_ptr<core::LRUHandle> handle;
    Error rs = GetOrLoadTable(cfd, file_number, 0, &handle);
    if (!rs) {
        return rs;
    }
    *props = GetEntry(handle.get())->table->GetTableProperties();
    return Error::OK();
}

Error TableCache::GetKeyFilter(const ColumnFamilyImpl *cfd, uint64_t file_number,
                               base::intrusive_ptr<core::KeyFilter> *filter) {
    base::intrusive_ptr<core::LRUHandle> handle;
    Error rs = GetOrLoadTable(cfd, file_number, 0, &handle);
    if (!rs) {
        return rs;
    }
    *filter = GetEntry(handle.get())->table->GetKeyFilter();
    return Error::OK();
}
    
Error TableCache::GetOrLoadTable(const ColumnFamilyImpl *cfd,
                                 uint64_t file_number, uint64_t file_size,
                                 base::intrusive_ptr<core::LRUHandle> *result) {
    Args args = std::make_tuple(cfd, file_number, file_size);
    return cache_.GetOrLoad(GetKey(&file_number), result, &EntryDeleter,
                            &EntryLoader, this, &args);
}
    
Error TableCache::LoadTable(const ColumnFamilyImpl *cfd,
                            uint64_t file_number, uint64_t file_size,
                            Entry *result) {
    
    result->file_name = cfd->GetTableFileName(file_number);
    Error rs = env_->NewRandomAccessFile(result->file_name, &result->file,
                                         allow_mmap_reads_);
    if (!rs) {
        return rs;
    }
    if (file_size == 0) {
        rs = result->file->GetFileSize(&file_size);
        if (!rs) {
            return rs;
        }
    }
    rs = factory_->NewTableReader(cfd->options().use_unordered_table ?
                                  "s1t" : "sst",
                                  cfd->ikcmp(),
                                  result->file.get(),
                                  file_number,
                                  file_size,
                                  true,
                                  block_cache_.get(),
                                  &result->table);
    if (!rs) {
        return rs;
    }
    result->cfid = cfd->id();
    return Error::OK();
}
    
} // namespace db
    
} // namespace mai
