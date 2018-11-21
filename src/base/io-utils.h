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
    
    static Error ReadAll(const std::string &file_name, std::string_view *result,
                         std::string *scatch, Env *env) {
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
        return file->Read(0, file_size, result, scatch);
    }

private:
    bool ownership_;
    SequentialFile *file_;
    
    std::string scratch_;
    size_t read_size_ = 0;
    Error error_;
}; // class FileReader
    
class FileWriter final {
public:
    FileWriter(WritableFile *file, bool ownership = false)
        : file_(DCHECK_NOTNULL(file))
        , ownership_(ownership) {}
    
    ~FileWriter() {
        if (ownership_) { delete file_; }
    }
    
    Error error() const { return error_; }
    
    void Write(std::string_view buf) {
        error_ = file_->Append(buf);
        written_size_ += buf.size();
    }
    
    void Write(const void *p, size_t n) {
        return Write(std::string_view(static_cast<const char *>(p), n));
    }
    
    void WriteByte(char b) { Write(std::string_view(&b, 1)); }
    
    void WriteFixed16(uint16_t value) {
        error_ = file_->Append(Slice::GetU16(value, &scope_));
        written_size_ += 2;
    }
    
    void WriteFixed32(uint32_t value) {
        error_ = file_->Append(Slice::GetU32(value, &scope_));
        written_size_ += 4;
    }
    
    void WriteFixed64(uint64_t value) {
        error_ = file_->Append(Slice::GetU64(value, &scope_));
        written_size_ += 8;
    }
    
    void WriteVarint32(uint32_t value) {
        char *p = static_cast<char *>(scope_.New(Varint32::kMaxLen));
        size_t n = Varint32::Encode(p, value);
        Write(std::string_view(p, n));
    }
    
    void WriteVarint64(uint64_t value) {
        char *p = static_cast<char *>(scope_.New(Varint64::kMaxLen));
        size_t n = Varint64::Encode(p, value);
        Write(std::string_view(p, n));
    }
    
    void Flush() { error_ = file_->Flush(); }
    
    void Sync(bool doit) {
        if (doit) {
            error_ = file_->Sync();
        }
    }
    
    DEF_VAL_GETTER(size_t, written_size);
    
    static Error WriteAll(const std::string &file_name, std::string_view data,
                          Env *env) {
        std::unique_ptr<WritableFile> file;
        Error rs = env->NewWritableFile(file_name, &file);
        if (!rs) {
            return rs;
        }
        return file->Append(data);
    }
    
private:
    bool ownership_;
    WritableFile *file_;
    
    ScopedMemory scope_;
    size_t written_size_ = 0;
    Error error_;
}; // class FileWriter

    
} // namespace base
    
} // namespace mai


#endif // MAI_BASE_IO_UTILS_H_
