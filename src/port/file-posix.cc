#include "port/file-posix.h"
#include <unistd.h>
#include <fcntl.h>

namespace mai {
    
namespace port {

/*virtual*/ WritableFilePosix::~WritableFilePosix() {}

/*static*/ Error WritableFilePosix::Open(const std::string &file_name,
                                         std::unique_ptr<WritableFile> *file) {
    int fd = ::open(file_name.c_str(), O_WRONLY|O_CREAT,
                    S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    if (fd < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    file->reset(new WritableFilePosix(fd));
    return Error::OK();
}

/*virtual*/ Error WritableFilePosix::Append(std::string_view data) {
    const char* src = data.data();
    size_t left = data.size();
    while (left != 0) {
        ssize_t done = write(fd_, src, left);
        if (done < 0) {
            if (errno == EINTR) {
                continue;
            }
            return MAI_IO_ERROR(strerror(errno));
        }
        left -= done;
        src += done;
    }
    filesize_ += data.size();
    return Error::OK();
}
    
/*virtual*/ Error WritableFilePosix::PositionedAppend(std::string_view data,
                                                      uint64_t offset) {
    const char* src = data.data();
    size_t left = data.size();
    while (left != 0) {
        ssize_t done = pwrite(fd_, src, left, static_cast<off_t>(offset));
        if (done < 0) {
            if (errno == EINTR) {
                continue;
            }
            return MAI_IO_ERROR(strerror(errno));
        }
        left -= done;
        offset += done;
        src += done;
    }
    filesize_ = offset;
    return Error::OK();
}
    
/*virtual*/ Error WritableFilePosix::Close() {
    if (::close(fd_) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    fd_ = -1;
    return Error::OK();
}
    
/*virtual*/ Error WritableFilePosix::Flush() {
    return Error::OK();
}
    
/*virtual*/ Error WritableFilePosix::Sync() {
    if (::fsync(fd_) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    return Error::OK();
}
    
MemRandomAccessFilePosix::~MemRandomAccessFilePosix() {}
    
} // namespace port
    
} // namespace mai
