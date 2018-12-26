#include "db/write-ahead-log.h"
#include "base/hash.h"

namespace mai {
    
namespace db {
    
LogWriter::LogWriter(WritableFile *file, size_t block_size)
    : writer_(file, false)
    , block_size_(block_size) {
    for (auto i = 0; i <= WAL::kMaxRecordType; i++) {
        uint8_t c = static_cast<uint8_t>(i);
        typed_checksums_[i] = ::crc32(0, &c, 1);
    }
}
    
#define TRY_RUN(expr) \
    rs = (expr); \
    if (rs.fail()) { \
        rs; \
    } (void)0
    
LogWriter::~LogWriter() {}

Error LogWriter::Append(std::string_view record) {
    size_t left = record.size();
    const char *p = record.data();
    Error rs;
    
    auto begin = true;
    do {
        size_t left_over = block_size_ - block_offset_;
        DCHECK_GE(left_over, 0);
        
        if (left_over < WAL::kHeaderSize) {
            if (left_over > 0) {
                TRY_RUN(writer_.Write("\x00\x00\x00\x00\x00\x00\x00",
                                       left_over));
            }
            block_offset_ = 0;
        }
        
        DCHECK_GE(block_size_ - block_offset_ - WAL::kHeaderSize, 0);
        
        const size_t avail = block_size_ - block_offset_ - WAL::kHeaderSize;
        const size_t fragment_length = (left < avail) ? left : avail;
        
        WAL::RecordType type;
        const bool end = (left == fragment_length);
        if (begin && end) {
            type = WAL::kFullType;
        } else if (begin) {
            type = WAL::kFirstType;
        } else if (end) {
            type = WAL::kLastType;
        } else {
            type = WAL::kMiddleType;
        }
        
        rs = EmitPhysicalRecord(p, fragment_length, type);
        p += fragment_length;
        left -= fragment_length;
        begin = false;
    } while (rs.ok() && left > 0);
    
    return rs;
}

Error LogWriter::EmitPhysicalRecord(const void *data, size_t len,
                                    WAL::RecordType type) {
    using ::mai::base::Slice;
    using ::mai::base::ScopedMemory;
    
    DCHECK_LE(len, UINT16_MAX);
    DCHECK_LE(block_offset_ + WAL::kHeaderSize + len, block_size_);
    Error rs;
    
    uint32_t checksum = ::crc32(typed_checksums_[type], data, len);
    
    TRY_RUN(writer_.WriteFixed32(checksum));
    TRY_RUN(writer_.WriteFixed16(static_cast<uint16_t>(len)));
    TRY_RUN(writer_.WriteByte(static_cast<uint8_t>(type)));
    TRY_RUN(writer_.Write(data, len));

    block_offset_ += (WAL::kHeaderSize + len);
    return Error::OK();
}

#undef TRY_RUN
    
LogReader::LogReader(SequentialFile *file, bool verify_checksum, size_t block_size)
    : reader_(file)
    , verify_checksum_(verify_checksum)
    , block_size_(block_size) {}

LogReader::~LogReader() {}
    
bool LogReader::Read(std::string_view *result, std::string* scratch) {
    if (reader_.error().IsEof()) {
        return false;
    }
    
    std::string buf;
    auto fail = 0, segment = 0;
    WAL::RecordType type = WAL::kZeroType;
    do {
        auto left_over = block_size_ - block_offset_;
        DCHECK_GE(left_over, 0);
        
        if (left_over < WAL::kHeaderSize) {
            if (left_over > 0) {
                reader_.Skip(left_over);
            }
            block_offset_ = 0;
        }
        
        if (segment > 0) {
            if (segment == 1) {
                scratch->clear();
            }
            scratch->append(result->data(), result->size());
            //scratch->append(buf);
        }
        
        type = ReadPhysicalRecord(result, &fail);
        segment++;
    } while (type == WAL::kMiddleType || type == WAL::kFirstType);
    
    if (fail > 0) {
        if (reader_.error().fail()) {
            error_ = reader_.error();
        } else {
            error_ = MAI_IO_ERROR("crc32 checksum fail.");
        }
    } else {
        error_ = Error::OK();
    }
    
    if (segment > 1) {
        scratch->append(result->data(), result->size());
        *result = *scratch;
    }
    return error_.ok();
}
    
#define TRY_RUN(expr) \
    (expr); \
    if (reader_.error().fail()) { \
        (*fail)++; \
        return static_cast<WAL::RecordType>(0); \
    }(void)0

WAL::RecordType LogReader::ReadPhysicalRecord(std::string_view *result,
                                              int *fail) {
    uint32_t record_checksum;
    uint16_t len;
    uint8_t type;
    
    TRY_RUN(record_checksum = reader_.ReadFixed32());
    TRY_RUN(len = reader_.ReadFixed16());
    TRY_RUN(type = reader_.ReadByte());
    
    TRY_RUN(*result = reader_.Read(len));
    
    if (verify_checksum_) {
        uint32_t checksum = ::crc32(0, &type, 1);
        checksum = ::crc32(checksum, result->data(), result->size());
        
        if (record_checksum != checksum) {
            (*fail)++;
        }
    }
    block_offset_ += (WAL::kHeaderSize + len);
    return static_cast<WAL::RecordType>(type);
}
    
} // namespace db

} // namespace mai
