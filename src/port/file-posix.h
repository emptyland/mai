#ifndef MAI_PORT_FILE_POSIX_H_
#define MAI_PORT_FILE_POSIX_H_

#include "mai/env.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {
    
namespace port {
    
class WritableFilePosix : public WritableFile {
public:
    virtual ~WritableFilePosix();
    
    static Error Open(const std::string &file_name,
                      std::unique_ptr<WritableFile> *file);
    
    virtual Error Append(std::string_view data) override;
    virtual Error PositionedAppend(std::string_view data, uint64_t offset) override;
    virtual Error Close() override;
    virtual Error Flush() override;
    virtual Error Sync() override;
    
    DISALLOW_ALL_CONSTRUCTORS(WritableFilePosix);
private:
    WritableFilePosix(int fd) : fd_(fd) {
        DCHECK_GE(fd, 0) << "Invalid fd!";
    }
    
    int fd_ = -1;
    size_t filesize_ = 0;
}; // class WritableFilePosix


class MemRandomAccessFilePosix : public RandomAccessFile {
public:
    virtual ~MemRandomAccessFilePosix();
    
private:
    MemRandomAccessFilePosix(int fd)
        : fd_(fd) {
        DCHECK_GE(fd_, 0);
    }

    int fd_ = -1;
}; // class MemRandomAccessFilePosix
    
} // namespace port
    
} // namespace mai

#endif // MAI_PORT_FILE_POSIX_H_
