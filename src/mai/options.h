#ifndef MAI_OPTIONS_H_
#define MAI_OPTIONS_H_

namespace mai {
    
class Snapshot;
    
    
struct Options {
    
}; // struct Options
    
struct ColumnFamilyOptions {
    
}; // struct ColumnFamilyOptions
    
struct ReadOptions {

    const Snapshot* snapshot = nullptr;
    
    bool verify_checksums = true;
    
}; // struct ReadOptions
    
struct WriteOptions {
    
}; // struct WriteOptions
    
} // namespace mai

#endif // MAI_OPTIONS_H_
