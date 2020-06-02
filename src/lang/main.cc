#include "mai/handle.h"
#include "mai/isolate.h"
#include "mai/value.h"
#include "mai/at-exit.h"
#include "glog/logging.h"
#include "gflags/gflags.h"

static constexpr size_t kMinNewGenSize = 20L * 1024L * 1024L;
static constexpr size_t kMinOldGenSize = 2L * 1024L * 1024L * 1024L;

DEFINE_int32(concurrency, 0, "How many concrrent coroutine running");
DEFINE_string(base_pkg_dir, "src/lang/pkg", "Language base pkg path");
DEFINE_uint64(new_gen_size, kMinNewGenSize, "New space initial size: 20MB");
DEFINE_uint64(old_gen_size, kMinOldGenSize, "Old space initial size: 2GB");
DEFINE_double(minor_gc_threshold_rate, 0.75, "Minor GC available threshold rate");
DEFINE_double(major_gc_threshold_rate, 0.25, "Major GC available threshold rate");


int Die(const char *message, const ::mai::Error *err) {
    if (err) {
        ::fprintf(stderr, "[ERR] %s: %s\n", message, err->ToString().c_str());
    } else {
        ::fprintf(stderr, "[ERR] %s\n", message);
    }
    return -1;
}

int main(int argc, char *argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 2;

    ::google::InitGoogleLogging(argv[0]);
    ::gflags::SetUsageMessage("Mai-Language");
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (argc < 2) {
        return Die("No file(s) input", nullptr);
    }
    
    ::mai::AtExit at_exit(::mai::AtExit::INITIALIZER);

    ::mai::lang::Options opts;
    opts.concurrency = FLAGS_concurrency;
    opts.base_pkg_dir = FLAGS_base_pkg_dir;
    
    if (FLAGS_new_gen_size < kMinNewGenSize) {
        printf("[WARN] new_gen_size is too small: %llu\n", FLAGS_new_gen_size);
        FLAGS_new_gen_size = kMinNewGenSize;
    }
    opts.new_space_initial_size = FLAGS_new_gen_size;
    
    if (FLAGS_old_gen_size < kMinOldGenSize) {
        printf("[WARN] old_gen_size is too small: %llu\n", FLAGS_old_gen_size);
        FLAGS_old_gen_size = kMinOldGenSize;
    }
    opts.old_space_limit_size = FLAGS_old_gen_size;
    opts.new_space_gc_threshold_rate = FLAGS_minor_gc_threshold_rate;
    opts.old_space_gc_threshold_rate = FLAGS_major_gc_threshold_rate;

    ::mai::lang::Isolate *isolate = ::mai::lang::Isolate::New(opts);
    if (!isolate) {
        return Die("Can not new isolate", nullptr);
    }
    if (::mai::Error err = isolate->Initialize(); !err.ok()) {
        return Die("Isolate initialize fail", &err);
    }
    
    ::mai::lang::IsolateScope isolate_scope(isolate);
    if (::mai::Error err = isolate->LoadBaseLibraries(); !err.ok()) {
        return Die("Load base libraries fail", &err);
    }

    ::mai::lang::HandleScope handle_scope(::mai::lang::HandleScope::INITIALIZER);
    
    if (::mai::Error err = isolate->Compile(argv[1]); err.fail()) {
        return Die("Compile fail", &err);
    }
    isolate->Run();
    return 0;
}

