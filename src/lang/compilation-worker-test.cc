#include "lang/compilation-worker.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class CompilationWorkerTest : public ::testing::Test {
public:
    CompilationWorkerTest() : worker_(new CompilationWorker(4)) {}
    
    //void TearDown() override { worker_.Shutdown(); }

    std::unique_ptr<CompilationWorker> worker_;
};

class DemoJob : public CompilationJob {
public:
    DemoJob(std::atomic<int> *count): count_(count) {}

    void Run() override { count_->fetch_add(1); }
    
private:
    std::atomic<int> *count_;
};

TEST_F(CompilationWorkerTest, Sanity) {
    std::atomic<int> count{0};
    
    for (int i = 0; i < 10000; i++) {
        worker_->Commit(new DemoJob(&count));
    }

    EXPECT_EQ(10000, worker_->Shutdown() + count.load());
}

} // namespace lang

} // namespace mai
