#ifndef MAI_DB_WRITE_AHEAD_LOG_H_
#define MAI_DB_WRITE_AHEAD_LOG_H_

#include "base/io-utils.h"

namespace mai {
class WritableFile;
namespace db {
    
struct WAL {
    enum RecordType {
        // Zero is reserved for preallocated files
        kZeroType = 0,
        
        kFullType = 1,
        
        // For fragments
        kFirstType = 2,
        kMiddleType = 3,
        kLastType = 4
    };
    
    static const int kMaxRecordType = kLastType;
    static const int kHeaderSize = 4 + 2 + 1;
    static const int kDefaultBlockSize = 32768;

    DISALLOW_ALL_CONSTRUCTORS(WAL);
}; // class WAL
    
/*
 * +---------+-------+
 * |         | crc32 | 4 bytes
 * |         +-------+
 * | header  | len   | 2 bytes
 * |         +-------+
 * |         | type  | 1 bytes
 * +---------+-------+
 * | payload | data  | len bytes
 * +---------+-------+
 */
class LogWriter {
public:
    LogWriter(WritableFile *file, size_t block_size);
    ~LogWriter();
    
    Error Append(std::string_view data);
    
    Error Flush() { return writer_.Flush(); }
    Error Sync(bool doit) { return writer_.Sync(doit); }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(LogWriter);
private:
    Error EmitPhysicalRecord(const void *buf, size_t len, WAL::RecordType type);
    
    const size_t block_size_;
    int block_offset_ = 0;
    
    uint32_t typed_checksums_[WAL::kMaxRecordType + 1];
    base::FileWriter writer_;
}; // class LogWriter
    
    
class LogReader {
public:
    LogReader(SequentialFile *file, bool verify_checksum, size_t block_size);
    ~LogReader();
    
    bool Read(std::string_view *result, std::string* scratch);
    
    Error error() const { return error_; }
private:
    WAL::RecordType ReadPhysicalRecord(std::string_view *result, int *fail);
    
    base::FileReader reader_;
    bool verify_checksum_;
    const size_t block_size_;
    
    Error error_;
    int block_offset_ = 0;
}; // class LogReader
    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_WRITE_AHEAD_LOG_H_
