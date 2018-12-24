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

DEFINE_int32(n_writers, 1, "How many threads for putting.");
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

void RunBenchmark(Env *env, DB *db) {
    std::thread *wr_thrds = FLAGS_n_writers < 2 ? nullptr
                            : new std::thread[FLAGS_n_writers - 1];
    std::atomic<size_t> written_bytes(0);
    
    auto jiffy = env->CurrentTimeMicros();
    for (int i = 1; i < FLAGS_n_writers; ++i) {
        wr_thrds[i - 1] = std::thread([&written_bytes, &db] (auto slot) {
            char key[124];
            std::string value;
            value.resize(FLAGS_value_size, 'F');
            
            for (int i = slot * FLAGS_count;
                 i < (slot + 1) * FLAGS_count; ++i) {
                snprintf(key, sizeof(key), "k-%08d", i);
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
    
    if (wr_thrds) {
        for (int i = 1; i < FLAGS_n_writers; ++i) {
            wr_thrds[i - 1].join();
        }
    }
    
    auto ms = (env->CurrentTimeMicros() - jiffy) / 1000.0;
    
    ::printf("Written Keys: %d\n", FLAGS_count * FLAGS_n_writers);
    ::printf("Total Size: %f MB\n", written_bytes.load() / (1024.0 * 1024.0));
    ::printf("Cost Time: %f ms\n", ms);
    ::printf("Write: %f MB/s %f op/s\n",
             written_bytes.load() / (1024.0 * 1024.0) / (ms / 1000.0),
             FLAGS_count * FLAGS_n_writers / (ms / 1000.0));

    Error rs = db->GetProperty("db.cf.default.levels", &value);
    ::printf("%s\n", value.c_str());

    delete[] wr_thrds;
}

int main(int argc, char *argv[]) {
    ::google::InitGoogleLogging(argv[0]);
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    
    Options options;
    options.create_if_missing = true;
    options.error_if_exists   = true;
    options.allow_mmap_reads  = FLAGS_allow_mmap_reads;
    
    ColumnFamilyDescriptor cf_desc;
    cf_desc.name = ::mai::kDefaultColumnFamilyName;
    cf_desc.options.use_unordered_table = false;
    cf_desc.options.write_buffer_size = FLAGS_write_buffer_size;
    
    std::string name(FLAGS_dir);
    name.append("/benchmark");
    
    DB *db;
    Error rs = DB::Open(options, name, {cf_desc}, nullptr, &db);
    if (!rs) {
        Die("Can not open db: " + name, rs);
    }
    
    RunBenchmark(Env::Default(), db);
    
    delete db;
    return 0;
}

