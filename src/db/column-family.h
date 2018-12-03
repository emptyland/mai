#ifndef MAI_DB_COLUMN_FAMILY_H_
#define MAI_DB_COLUMN_FAMILY_H_

#include "db/config.h"
#include "core/memory-table.h"
#include "core/internal-key-comparator.h"
#include "core/pipeline-queue.h"
#include "base/reference-count.h"
#include "base/spin-locking.h"
#include "base/base.h"
#include "mai/db.h"
#include "glog/logging.h"
#include <unordered_map>
#include <condition_variable>
#include <atomic>

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
struct CompactionContext;

class ColumnFamilyImpl final {
public:
    using ImmutablePipeline =
        core::PipelineQueue<base::intrusive_ptr<core::MemoryTable>>;
    
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
    
    Error Install(Factory *factory);
    Error Uninstall();
    
    std::string GetDir() const;
    std::string GetTableFileName(uint64_t file_number) const;
    
    void Drop();
    bool dropped() const { return dropped_.load(); }
    
    Error AddIterators(const ReadOptions &opts, std::vector<Iterator *> *result);
    
    DEF_VAL_GETTER(std::string, name);
    DEF_VAL_GETTER(uint32_t, id);
    DEF_PTR_GETTER(ColumnFamilyImpl, next);
    DEF_PTR_GETTER(ColumnFamilyImpl, prev);
    DEF_VAL_GETTER(ColumnFamilyOptions, options);
    DEF_PTR_GETTER_NOTNULL(ColumnFamilySet, owns);
    DEF_PTR_GETTER_NOTNULL(Version, current);
    DEF_VAL_GETTER(bool, initialized);
    DEF_VAL_PROP_RW(Error, background_error);
    DEF_VAL_MUTABLE_GETTER(std::condition_variable, background_cv);
    DEF_VAL_PROP_RW(uint64_t, redo_log_number);
    
    void set_background_progress(bool value) {
        background_progress_.store(value);
    }
    
    bool background_progress() const {
        return background_progress_.load();
    }
    
    core::MemoryTable *mutable_table() const { return mutable_.get(); }
    ImmutablePipeline *immutable_pipeline() { return &immutable_pipeline_; }
    
    std::string compaction_point(int level) const {
        DCHECK_GE(level, 0);
        DCHECK_LT(level, Config::kMaxLevel);
        return compaction_point_[level];
    }
    
    void MakeImmutablePipeline(Factory *factory, uint64_t redo_log_number);
    void Append(Version *version);
    bool NeedsCompaction() const;
    bool PickCompaction(CompactionContext *ctx);
    
    const core::InternalKeyComparator *ikcmp() const { return &ikcmp_; }

    friend class ColumnFamilySet;
    friend class VersionSet;
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnFamilyImpl);
private:
    void SetupOtherInputs(CompactionContext *ctx);
    
    const std::string name_;
    const uint32_t id_;
    const ColumnFamilyOptions options_;
    ColumnFamilySet *const owns_;
    mutable std::atomic<int> ref_count_;
    std::atomic<bool> dropped_;
    Version *dummy_versions_;
    std::atomic<bool> background_progress_;
    
    Version *current_ = nullptr;
    bool initialized_ = false;
    
    const core::InternalKeyComparator ikcmp_;
    base::intrusive_ptr<core::MemoryTable> mutable_;
    core::PipelineQueue<base::intrusive_ptr<core::MemoryTable>> immutable_pipeline_;
    
    Error background_error_;
    std::condition_variable background_cv_;

    std::string compaction_point_[Config::kMaxLevel];
    
    // log file number
    uint64_t redo_log_number_ = 0;

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
    
    static ColumnFamilyHandle *Cast(ColumnFamily *cf) {
        if (!cf) { return nullptr; }
        return down_cast<ColumnFamilyHandle>(cf);
    }
private:
    DBImpl *const db_;
    base::intrusive_ptr<ColumnFamilyImpl> impl_;
}; // class ColumnFamilyHandle

class ColumnFamilySet final {
public:
    ColumnFamilySet(VersionSet *owns, TableCache *table_cache);
    ~ColumnFamilySet();
    
    const std::string &abs_db_path() const;
    Env *env() const;
    
    DEF_PTR_GETTER_NOTNULL(VersionSet, owns);
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnFamilySet);
private:
    VersionSet *const owns_;
    TableCache *const table_cache_;

    ColumnFamilyImpl *dummy_cfd_;
    uint32_t max_column_family_ = 0;
    ColumnFamilyImpl *default_cfd_ = nullptr;
    std::unordered_map<std::string, uint32_t> column_families_;
    std::unordered_map<uint32_t, ColumnFamilyImpl *> column_family_impls_;
}; // class ColumnFamilySet
    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_COLUMN_FAMILY_H_
