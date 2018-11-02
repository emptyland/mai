#ifndef MAI_OPTIONS_H_
#define MAI_OPTIONS_H_

namespace mai {
    
class Snapshot;
    
struct ReadOptions {

    const Snapshot* snapshot = nullptr;
    
    bool verify_checksums = true;
    
}; // struct ReadOptions
    
} // namespace mai

#endif // MAI_OPTIONS_H_
