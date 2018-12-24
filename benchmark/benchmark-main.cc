#include "mai/db.h"
#include "mai/env.h"
#include "mai/helper.h"
#include "mai/options.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include <stdio.h>
#include <thread>


using ::mai::DB;
using ::mai::Env;
using ::mai::ColumnFamily;
using ::mai::ColumnFamilyOptions;
using ::mai::ColumnFamilyDescriptor;
using ::mai::Options;
using ::mai::WriteOptions;
using ::mai::ReadOptions;
using ::mai::ColumnFamilyOptions;
using ::mai::ColumnFamilyCollection;
using ::mai::Error;

DEFINE_bool(read, false, "Run reading benchmark.");
DEFINE_int32(n_workers, 1, "How many threads for putting.");
DEFINE_int32(value_size, 128, "Value size(bytes).");
DEFINE_int32(count, 1, "Benchmark runing count.");
DEFINE_string(dir, "./tests", "Benchmark running dir.");
DEFINE_bool(allow_mmap_reads, false, "Use mmap reading.");
DEFINE_bool(use_unordered_table, false, "Use hash table.");
DEFINE_int64(write_buffer_size, 40 * 1024 * 1024,
             "Key-Value entries In-memory buffer.");

void Die(const std::string &msg, const Error &rs) {
    ::fprintf(stderr, "%s: \nCause: %s\n", msg.c_str(), rs.ToString().c_str());
    ::exit(-1);
}

void RunWriteBenchmark(Env *env, DB *db) {
    std::thread *wr_thrds = FLAGS_n_workers < 2 ? nullptr
                            : new std::thread[FLAGS_n_workers - 1];
    std::atomic<size_t> written_bytes(0);
    
    auto jiffy = env->CurrentTimeMicros();
    for (int i = 1; i < FLAGS_n_workers; ++i) {
        wr_thrds[i - 1] = std::thread([&written_bytes, &db] (auto slot) {
            char key[124];
            std::string value;
            value.resize(FLAGS_value_size, 'F');
            
            for (int i = slot * FLAGS_count;
                 i < (slot + 1) * FLAGS_count; ++i) {
                ::snprintf(key, sizeof(key), "k-%08d", i);
                Error rs = db->Put(WriteOptions{}, db->DefaultColumnFamily(), key, value);
                if (!rs) {
                    Die("Put fail!", rs);
                }
                written_bytes.fetch_add(strlen(key) + value.size());
            }
        }, i);
    }
    
    char key[124];
    std::string value;
    value.resize(FLAGS_value_size, 'F');
    for (int i = 0; i < FLAGS_count; ++i) {
        snprintf(key, sizeof(key), "k-%08d", i);
        Error rs = db->Put(WriteOptions{}, db->DefaultColumnFamily(), key, value);
        if (!rs) {
            Die("Put fail!", rs);
        }
        written_bytes.fetch_add(strlen(key) + value.size());
    }
    
    for (int i = 1; i < FLAGS_n_workers; ++i) {
        wr_thrds[i - 1].join();
    }
    
    auto ms = (env->CurrentTimeMicros() - jiffy) / 1000.0;
    
    ::printf("Written Keys: %d\n", FLAGS_count * FLAGS_n_workers);
    ::printf("Total Size: %f MB\n", written_bytes.load() / (1024.0 * 1024.0));
    ::printf("Cost Time: %f ms\n", ms);
    ::printf("Write: %f MB/s %f op/s\n",
             written_bytes.load() / (1024.0 * 1024.0) / (ms / 1000.0),
             FLAGS_count * FLAGS_n_workers / (ms / 1000.0));

    Error rs = db->GetProperty("db.cf.default.levels", &value);
    ::printf("%s\n", value.c_str());

    delete[] wr_thrds;
}

void RunReadBenchmark(Env *env, DB *db) {
    std::thread *rd_thrds = FLAGS_n_workers < 2 ? nullptr
                          : new std::thread[FLAGS_n_workers - 1];
    std::atomic<size_t> read_bytes(0);

    auto jiffy = env->CurrentTimeMicros();
    for (int i = 1; i < FLAGS_n_workers; ++i) {
        rd_thrds[i - 1] = std::thread([&read_bytes, &db] (auto slot) {
            char key[124];
            std::string value;
            
            for (int i = slot * FLAGS_count;
                 i < (slot + 1) * FLAGS_count; ++i) {
                ::snprintf(key, sizeof(key), "k-%08d", i);
                Error rs = db->Get(ReadOptions{}, db->DefaultColumnFamily(), key, &value);
                if (!rs) {
                    Die("Get fail!", rs);
                }
                read_bytes.fetch_add(strlen(key) + value.size());
            }
        }, i);
    }
    
    char key[124];
    //strncpy(key, "k-00222448", sizeof(key));
    std::string value;
    
    for (int i = 0; i < FLAGS_count; ++i) {
        ::snprintf(key, sizeof(key), "k-%08d", i);
        Error rs = db->Get(ReadOptions{}, db->DefaultColumnFamily(), key, &value);
        if (!rs) {
            Die("Get fail!", rs);
        }
        read_bytes.fetch_add(strlen(key) + value.size());
    }
    
    for (int i = 1; i < FLAGS_n_workers; ++i) {
        rd_thrds[i - 1].join();
    }
    
    auto ms = (env->CurrentTimeMicros() - jiffy) / 1000.0;
    
    ::printf("Read Keys: %d\n", FLAGS_count * FLAGS_n_workers);
    ::printf("Total Size: %f MB\n", read_bytes.load() / (1024.0 * 1024.0));
    ::printf("Cost Time: %f ms\n", ms);
    ::printf("Read: %f MB/s %f op/s\n",
             read_bytes.load() / (1024.0 * 1024.0) / (ms / 1000.0),
             FLAGS_count * FLAGS_n_workers / (ms / 1000.0));
    delete[] rd_thrds;
}

int main(int argc, char *argv[]) {
    FLAGS_logtostderr = 1;
    FLAGS_minloglevel = 3;
    
    ::google::InitGoogleLogging(argv[0]);
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    
    Options options;
    options.create_if_missing = true;
    options.error_if_exists   = !FLAGS_read;
    options.allow_mmap_reads  = FLAGS_allow_mmap_reads;
    
    ColumnFamilyDescriptor cf_desc;
    cf_desc.name = ::mai::kDefaultColumnFamilyName;
    cf_desc.options.use_unordered_table = FLAGS_use_unordered_table;
    cf_desc.options.write_buffer_size   = FLAGS_write_buffer_size;
    cf_desc.options.block_size          = 16384;
    
    std::string name(FLAGS_dir);
    name.append("/benchmark");
    
    DB *db;
    Error rs = DB::Open(options, name, {cf_desc}, nullptr, &db);
    if (!rs) {
        Die("Can not open db: " + name, rs);
    }
    
    if (FLAGS_read) {
        RunReadBenchmark(Env::Default(), db);
    } else {
        RunWriteBenchmark(Env::Default(), db);
    }
    
    delete db;
    return 0;
}

