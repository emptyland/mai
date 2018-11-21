#ifndef MAI_DB_COLUMN_FAMILY_H_
#define MAI_DB_COLUMN_FAMILY_H_

#include "base/reference-count.h"
#include "base/base.h"
#include "core/memory-table.h"
#include "core/internal-key-comparator.h"
#include "base/spin-locking.h"
#include "mai/db.h"
#include "glog/logging.h"
#include <unordered_map>

namespace mai {
    
namespace db {
    
class DBImpl;
class Version;
class VersionSet;
class Factory;
class TableCache;
class ColumnFamilySet;
class ColumnFamilyImpl;
class ColumnFamilyHandle;

class ColumnFamilyImpl final {
public:
    ColumnFamilyImpl(const std::string &name, uint32_t id,
                     const ColumnFamilyOptions &options,
                     VersionSet *versions,
                     ColumnFamilySet *onwer);
    ~ColumnFamilyImpl();
    
    void AddRef() const { ref_count_.fetch_add(1, std::memory_order_relaxed); }
    
    bool ReleaseRef() const {
        int old_val = ref_count_.fetch_sub(1, std::memory_order_relaxed);
        DCHECK_GT(old_val, 0);
        return old_val == 1;
    }
    
    int ref_count() const { return ref_count_.load(std::memory_order_relaxed); }
    
    Error Install(Factory *factory, Env *env);
    
    std::string GetDir() const;
    std::string GetTableFileName(uint64_t file_number) const;
    
    void SetDropped();
    bool IsDropped() const { return dropped_.load(); }
    
    Error AddIterators(const ReadOptions &opts, std::vector<Iterator *> *result);
    
    DEF_VAL_GETTER(std::string, name);
    DEF_VAL_GETTER(uint32_t, id);
    DEF_PTR_GETTER(ColumnFamilyImpl, next);
    DEF_PTR_GETTER(ColumnFamilyImpl, prev);
    DEF_VAL_GETTER(ColumnFamilyOptions, options);
    DEF_PTR_GETTER_NOTNULL(ColumnFamilySet, owns);
    DEF_PTR_GETTER_NOTNULL(Version, current);
    DEF_VAL_GETTER(bool, initialized);
    
    core::MemoryTable *mutable_table() const { return mutable_.get(); }
    core::MemoryTable *immutable_table() const { return immutable_.get(); }
    
    void Append(Version *version);
    
    const core::InternalKeyComparator *ikcmp() const { return &ikcmp_; }

    friend class ColumnFamilySet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnFamilyImpl);
private:
    const std::string name_;
    const uint32_t id_;
    const ColumnFamilyOptions options_;
    ColumnFamilySet *const owns_;
    mutable std::atomic<int> ref_count_;
    std::atomic<bool> dropped_;
    Version *dummy_versions_;
    
    Version *current_ = nullptr;
    bool initialized_ = false;
    
    const core::InternalKeyComparator ikcmp_;
    base::Handle<core::MemoryTable> mutable_;
    base::Handle<core::MemoryTable> immutable_;
    
    ColumnFamilyImpl *next_ = nullptr;
    ColumnFamilyImpl *prev_ = nullptr;
}; // class ColumnFamilyImpl
    
class ColumnFamilyHandle final : public ColumnFamily {
public:
    ColumnFamilyHandle(DBImpl *db, ColumnFamilyImpl *impl)
        : db_(DCHECK_NOTNULL(db))
        , impl_(DCHECK_NOTNULL(impl)) {}
    virtual ~ColumnFamilyHandle();
    
    virtual std::string name() const override;
    virtual uint32_t id() const override;
    virtual const Comparator *comparator() const override;
    virtual Error GetDescriptor(ColumnFamilyDescriptor *desc) const override;
    
    DEF_PTR_GETTER_NOTNULL(DBImpl, db);
    ColumnFamilyImpl *impl() const { return impl_.get(); }
private:
    DBImpl *const db_;
    base::Handle<ColumnFamilyImpl> impl_;
}; // class ColumnFamilyHandle

class ColumnFamilySet final {
public:
    ColumnFamilySet(const std::string &db_name, TableCache *table_cache);
    ~ColumnFamilySet();
    
    DEF_VAL_GETTER(std::string, db_name);
    DEF_PTR_GETTER_NOTNULL(TableCache, table_cache);
    
    ColumnFamilyImpl *GetDefault() const { return DCHECK_NOTNULL(default_cfd_); }
    
    ColumnFamilyImpl *GetColumnFamily(uint32_t id) const {
        auto iter = column_family_impls_.find(id);
        if (iter == column_family_impls_.end()) {
            return nullptr;
        }
        return iter->second;
    }
    
    ColumnFamilyImpl *GetColumnFamily(const std::string &name) const {
        auto iter = column_families_.find(name);
        if (iter == column_families_.end()) {
            return nullptr;
        }
        return GetColumnFamily(iter->second);
    }
    
    uint32_t NextColumnFamilyId() { return ++max_column_family_; }
    uint32_t max_column_family() const { return max_column_family_; }
    void UpdateColumnFamilyId(uint32_t new_id) {
        max_column_family_ = std::max(new_id, max_column_family_);
    }
    size_t n_column_families() const { return column_families_.size(); }

    ColumnFamilyImpl *NewColumnFamily(const ColumnFamilyOptions opts,
                                      const std::string &name, uint32_t id,
                                      VersionSet *versions);
    
    void RemoveColumnFamily(ColumnFamilyImpl *cfd) {
        auto iter = column_family_impls_.find(cfd->id());
        DCHECK(iter != column_family_impls_.end());
        column_family_impls_.erase(iter);
        column_families_.erase(cfd->name());
    }
    
    class iterator {
    public:
        iterator(ColumnFamilyImpl *node) : node_(node) {}
        void operator ++ () { node_ = node_->next(); }
        void operator ++ (int) { node_ = node_->next(); }
        void operator -- () { node_ = node_->prev(); }
        ColumnFamilyImpl *operator *() const { return node_; }
        bool operator == (const iterator &iter) const {
            return node_ == iter.node_;
        }
        bool operator != (const iterator &iter) const {
            return node_ != iter.node_;
        }
    private:
        ColumnFamilyImpl *node_;
    }; // class iterator
    
    iterator begin() const { return iterator(dummy_cfd_->next()); }
    iterator end() const { return iterator(dummy_cfd_); }
    
    base::SpinRwMutex *rw_mutex() { return &rw_mutex_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnFamilySet);
private:
    const std::string db_name_;
    TableCache *const table_cache_;
    ColumnFamilyImpl *dummy_cfd_;
    uint32_t max_column_family_ = 0;
    ColumnFamilyImpl *default_cfd_ = nullptr;
    std::unordered_map<std::string, uint32_t> column_families_;
    std::unordered_map<uint32_t, ColumnFamilyImpl *> column_family_impls_;
    base::SpinRwMutex rw_mutex_;
}; // class ColumnFamilySet
    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_COLUMN_FAMILY_H_
