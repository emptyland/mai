#ifndef MAI_PORT_FILE_POSIX_H_
#define MAI_PORT_FILE_POSIX_H_

#include "mai/env.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace port {
    
class WritableFilePosix final : public WritableFile {
public:
    virtual ~WritableFilePosix();
    
    static Error Open(const std::string &file_name,
                      std::unique_ptr<WritableFile> *file);
    
    virtual Error Append(std::string_view data) override;
    virtual Error PositionedAppend(std::string_view data, uint64_t offset) override;
    virtual Error Close() override;
    virtual Error Flush() override;
    virtual Error Sync() override;
    virtual Error GetFileSize(uint64_t *size) override;
    virtual Error Truncate(uint64_t size) override;
    
    DISALLOW_ALL_CONSTRUCTORS(WritableFilePosix);
private:
    WritableFilePosix(int fd) : fd_(fd) {
        DCHECK_GE(fd, 0) << "Invalid fd!";
    }
    
    int fd_ = -1;
    size_t filesize_ = 0;
}; // class WritableFilePosix


class MemRandomAccessFilePosix final : public RandomAccessFile {
public:
    virtual ~MemRandomAccessFilePosix();
    
    static Error Open(const std::string &file_name,
                      std::unique_ptr<RandomAccessFile> *file);
    
    virtual Error Read(uint64_t offset, size_t n, std::string_view *result,
                       std::string *scratch) override;
    virtual Error GetFileSize(uint64_t *size) override;
    virtual Error Close() override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MemRandomAccessFilePosix);
private:
    MemRandomAccessFilePosix(int fd, uint64_t file_size, char *mapped)
        : fd_(fd)
        , file_size_(file_size)
        , mapped_(DCHECK_NOTNULL(mapped)) {
        DCHECK_GE(fd_, 0);
    }

    int fd_ = -1;
    uint64_t file_size_ = 0;
    char *mapped_ = nullptr;
}; // class MemRandomAccessFilePosix
    
} // namespace port
    
} // namespace mai

#endif // MAI_PORT_FILE_POSIX_H_
