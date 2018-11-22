#include "mai/env.h"
#include <chrono>

namespace mai {
    
Env::Env() {}
    
/*virtual*/ Env::~Env() {}
    
/*virtual*/ uint64_t Env::CurrentTimeMicros() {
    using namespace std::chrono;
    
    auto now = high_resolution_clock::now();
    return duration_cast<microseconds>(now.time_since_epoch()).count();
}

/*virtual*/ WritableFile::~WritableFile() {}
    
/*virtual*/ RandomAccessFile::~RandomAccessFile() {}
    
} // namespace mai
