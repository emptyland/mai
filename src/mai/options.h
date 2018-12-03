#ifndef MAI_OPTIONS_H_
#define MAI_OPTIONS_H_

#include "mai/env.h"
#include "mai/comparator.h"

namespace mai {
    
class Snapshot;
    
struct ColumnFamilyOptions {
    
    // Can use hash table?
    bool use_unordered_table = false;
    
    // Only use for hash table
    size_t number_of_hash_slots = 1024 * 100 + 1;
    
    // Only use for hash table: Dump l0 table if
    // conflict-factor > conflict_factor_limit
    float conflict_factor_limit = 3.0;
    
    // Should fixed the hash table slots size?
    bool fixed_number_of_slots = false;
    
    // 40MB
    size_t write_buffer_size = 40 * 1024 * 1024;
    
    // 4KB
    size_t block_size = 4 * 1024;
    
    // Only use for sst table
    int block_restart_interval = 16;
    
    std::string dir;
    
    const Comparator* comparator = Comparator::Bytewise();
}; // struct ColumnFamilyOptions
    
struct ReadOptions final {

    const Snapshot* snapshot = nullptr;
    
    bool verify_checksums = true;
    
}; // struct ReadOptions
    
struct WriteOptions final {
    
    bool sync = false;
    
}; // struct WriteOptions
    
struct Options final : public ColumnFamilyOptions {
    
    Env *env = Env::Default();
    
    bool create_if_missing = false;
    
    bool create_missing_column_families = false;
    
    bool error_if_exists = false;
    
    int max_open_files = 1000;
    
    // 80 MB
    size_t max_total_wal_size = 80 * 1024 * 1024;
    
    bool allow_mmap_reads = false;
    
    bool allow_mmap_writes = false;
}; // struct Options
    
} // namespace mai

#endif // MAI_OPTIONS_H_
