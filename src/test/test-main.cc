#include "mai/at-exit.h"
#include "gtest/gtest.h"
#include "glog/logging.h"

int main(int argc, char *argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 2;
    
    ::google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    ::mai::AtExit at_exit(::mai::AtExit::INITIALIZER);
    return RUN_ALL_TESTS();
}
