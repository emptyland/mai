#ifndef MAI_BASE_IO_UTILS_H_
#define MAI_BASE_IO_UTILS_H_

#include "mai/env.h"
#include "base/slice.h"

namespace mai {
    
namespace base {
    
class FileReader {
public:
    FileReader(SequentialFile *file, bool ownership = false)
        : file_(DCHECK_NOTNULL(file))
        , ownership_(ownership) {}
    
    ~FileReader() {
        if (ownership_) { delete file_; }
    }
    
    Error error() const { return error_; }
    
    std::string_view Read(size_t n) { return Read(n, &scratch_); }
    
    std::string_view Read(size_t n, std::string *scratch) {
        std::string_view result;
        error_ = file_->Read(n, &result, scratch);
        if (error_.IsEof()) {
            scratch->resize(n, '\0');
            result = *scratch;
        } else if (error_.ok()) {
            read_size_ += result.size();
        }
        return result;
    }
    
    uint8_t ReadByte() {
        std::string_view result = Read(1, &scratch_);
        return result[0];
    }
    
    uint16_t ReadFixed16() { return Slice::SetU16(Read(2, &scratch_)); }
    uint32_t ReadFixed32() { return Slice::SetU32(Read(4, &scratch_)); }
    uint64_t ReadFixed64() { return Slice::SetU64(Read(8, &scratch_)); }
    
    void Skip(size_t n) {
        error_ = file_->Skip(n);
        if (error_.ok()) {
            read_size_ += n;
        }
    }
    
    static Error ReadAll(const std::string &file_name, std::string *result,
                         Env *env) {
        std::unique_ptr<RandomAccessFile> file;
        Error rs = env->NewRandomAccessFile(file_name, &file);
        if (!rs) {
            return rs;
        }
        uint64_t file_size;
        rs = file->GetFileSize(&file_size);
        if (!rs) {
            return rs;
        }
        std::string_view slice;
        rs = file->Read(0, file_size, &slice, result);
        if (!rs) {
            return rs;
        }
        result->assign(slice);
        return Error::OK();
    }

private:
    bool ownership_;
    SequentialFile *file_;
    
    std::string scratch_;
    size_t read_size_ = 0;
    Error error_;
}; // class FileReader


class RandomAccessFileReader final {
public:
    RandomAccessFileReader(RandomAccessFile *file, bool ownership = false)
        : file_(DCHECK_NOTNULL(file))
        , ownership_(ownership) {}
    
    ~RandomAccessFileReader() {
        if (ownership_) { delete file_; }
    }
    
    DEF_VAL_GETTER(Error, error);
    DEF_PTR_GETTER_NOTNULL(RandomAccessFile, file);
    
    std::string_view Read(uint64_t offset, size_t n, std::string *scratch) {
        std::string_view result;
        error_ = file_->Read(offset, n, &result, scratch);
        if (error_.IsEof()) {
            read_size_ += result.size();
        } else if (error_.fail()) {
            scratch->resize(n, '\0');
            result = *scratch;
        } else if (error_.ok()) {
            read_size_ += result.size();
        }
        return result;
    }
    
    std::string_view Read(uint64_t offset, size_t n) {
        return Read(offset, n, &scratch_);
    }
    uint8_t ReadByte(uint64_t offset) {
        std::string_view result = Read(offset, 1, &scratch_);
        return result[0];
    }
    uint16_t ReadFixed16(uint64_t offset) {
        return Slice::SetU16(Read(offset, 2, &scratch_));
    }
    uint32_t ReadFixed32(uint64_t offset) {
        return Slice::SetU32(Read(offset, 4, &scratch_));
    }
    uint64_t ReadFixed64(uint64_t offset) {
        return Slice::SetU64(Read(offset, 8, &scratch_));
    }
    uint32_t ReadVarint32(uint64_t offset, size_t *len = nullptr) {
        std::string_view result = Read(offset, Varint32::kMaxLen, &scratch_);
        size_t varint_len;
        uint32_t value = Varint32::Decode(result.data(), &varint_len);
        if (len) { *len = varint_len; }
        return value;
    }
    uint64_t ReadVarint64(uint64_t offset, size_t *len = nullptr) {
        std::string_view result = Read(offset, Varint64::kMaxLen, &scratch_);
        size_t varint_len;
        uint64_t value = Varint64::Decode(result.data(), &varint_len);
        if (len) { *len = varint_len; }
        return value;
    }
    
private:
    bool ownership_;
    RandomAccessFile *file_;

    std::string scratch_;
    size_t read_size_ = 0;
    Error error_;
}; // class RandomAccessFileReader


class FileWriter final {
public:
    FileWriter(WritableFile *file, bool ownership = false)
        : file_(DCHECK_NOTNULL(file))
        , ownership_(ownership) {}
    
    ~FileWriter() {
        if (ownership_) { delete file_; }
    }
    
    Error Write(std::string_view buf) {
        Error rs = file_->Append(buf);
        if (rs.ok()) {
            written_position_ += buf.size();
        }
        return rs;
    }
    
    Error WritePad(size_t n) { return Write(Slice::GetPad(n, &scope_)); }
    
    Error Write(const void *p, size_t n) {
        return Write(std::string_view(static_cast<const char *>(p), n));
    }
    
    Error WriteByte(char b) { return Write(std::string_view(&b, 1)); }
    
    Error WriteFixed16(uint16_t value) {
        return Write(Slice::GetU16(value, &scope_));
    }
    
    Error WriteFixed32(uint32_t value) {
        return Write(Slice::GetU32(value, &scope_));
    }
    
    Error WriteFixed64(uint64_t value) {
        return Write(Slice::GetU64(value, &scope_));
    }
    
    Error WriteVarint32(uint32_t value) {
        char *p = static_cast<char *>(scope_.New(Varint32::kMaxLen));
        size_t n = Varint32::Encode(p, value);
        return Write(std::string_view(p, n));
    }
    
    Error WriteVarint64(uint64_t value) {
        char *p = static_cast<char *>(scope_.New(Varint64::kMaxLen));
        size_t n = Varint64::Encode(p, value);
        return Write(std::string_view(p, n));
    }
    
    Error Flush() { return file_->Flush(); }
    
    Error Sync(bool doit) {
        if (doit) {
            return file_->Sync();
        } else {
            return Error::OK();
        }
    }
    
    Error Truncate(uint64_t size) {
        Error rs = file_->Truncate(size);
        if (!rs) {
            return rs;
        }
        written_position_ = size;
        return Error::OK();
    }
    
    DEF_VAL_GETTER(size_t, written_position);
    DEF_PTR_GETTER_NOTNULL(WritableFile, file);
    
    static Error WriteAll(const std::string &file_name, std::string_view data,
                          Env *env) {
        std::unique_ptr<WritableFile> file;
        Error rs = env->NewWritableFile(file_name, false, &file);
        if (!rs) {
            return rs;
        }
        return file->Append(data);
    }
    
private:
    bool ownership_;
    WritableFile *file_;
    
    ScopedMemory scope_;
    size_t written_position_ = 0;
}; // class FileWriter

    
class BufferedWritableFile final : public WritableFile {
public:
    static const int kBufferSize = 16 * base::kKB;
    
    BufferedWritableFile(WritableFile *file, bool ownership)
        : file_(DCHECK_NOTNULL(file))
        , ownership_(ownership) {}
    
    virtual ~BufferedWritableFile() {
        if (!buf_.empty()) {
            Error rs = file_->Append(buf_);
            if (!rs) {
                LOG(ERROR) << "Flush file fail!";
            }
        }
        if (ownership_) { delete file_; }
    }
    
    virtual Error Append(std::string_view data) override {
        Error rs;
        if (data.size() >= kBufferSize) {
            rs = Flush();
            if (rs.ok()) {
                rs = file_->Append(data);
            }
        } else {
            if (buf_.size() + data.size() > kBufferSize) {
                rs = Flush();
            }
            if (rs.ok()) {
                buf_.append(data);
            }
        }
        return rs;
    }
    
    virtual Error PositionedAppend(std::string_view data,
                                   uint64_t offset) override {
        Error rs = Flush();
        if (!rs) {
            return rs;
        }
        return file_->PositionedAppend(data, offset);
    }
    
    virtual Error Flush() override {
        Error rs;
        if (!buf_.empty()) {
            rs = file_->Append(buf_);
            buf_.clear();
        }
        return rs;
    }
    
    virtual Error Sync() override { return file_->Sync(); }
    
    virtual Error GetFileSize(uint64_t *size) override {
        Error rs = file_->GetFileSize(size);
        if (!rs) {
            return rs;
        }
        *size += buf_.size();
        return Error::OK();
    }
    
    virtual Error Truncate(uint64_t size) override {
        Error rs = Flush();
        if (!rs) {
            return rs;
        }
        return file_->Truncate(size);
    }

private:
    WritableFile *const file_;
    const bool ownership_;
    std::string buf_;
}; // class BufferedWritableFile
    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_IO_UTILS_H_
