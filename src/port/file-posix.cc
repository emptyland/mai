#include "port/file-posix.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

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
    
/*virtual*/ Error WritableFilePosix::GetFileSize(uint64_t *size)  {
    struct stat s;
    if (::fstat(fd_, &s) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    *size = s.st_size;
    return Error::OK();
}
    
////////////////////////////////////////////////////////////////////////////////
/// class MemRandomAccessFilePosix
////////////////////////////////////////////////////////////////////////////////
    
MemRandomAccessFilePosix::~MemRandomAccessFilePosix() {}
    
/*static*/ Error
MemRandomAccessFilePosix::Open(const std::string &file_name,
                               std::unique_ptr<RandomAccessFile> *file) {
    int fd = ::open(file_name.c_str(), O_RDONLY);
    if (fd < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    
    struct stat s;
    if (::fstat(fd, &s) < 0) {
        close(fd);
        return MAI_IO_ERROR(strerror(errno));
    }
    
    void *mapped = ::mmap(nullptr, s.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE,
                          fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return MAI_IO_ERROR(strerror(errno));
    }
    
    file->reset(new MemRandomAccessFilePosix(fd, s.st_size,
                                             static_cast<char *>(mapped)));
    return Error::OK();
}

/*virtual*/ Error MemRandomAccessFilePosix::Read(uint64_t offset, size_t n,
                                                 std::string_view *result,
                                                 std::string */*scratch*/) {
    if (offset > file_size_) {
        return MAI_IO_ERROR("offset out of range!");
    }
    n = (file_size_ - offset < n ? file_size_ - offset : n);
    *result = std::string_view(mapped_ + offset, n);
    return Error::OK();
}

/*virtual*/ Error MemRandomAccessFilePosix::GetFileSize(uint64_t *size) {
    *size = file_size_;
    return Error::OK();
}
    
/*virtual*/ Error MemRandomAccessFilePosix::Close() {
    if (static_cast<void *>(mapped_) != MAP_FAILED) {
        if (::munmap(mapped_, file_size_) < 0) {
            return MAI_IO_ERROR(strerror(errno));
        }
        mapped_ = nullptr;
    }
    
    if (fd_ != -1) {
        if (::close(fd_) < 0) {
            return MAI_IO_ERROR(strerror(errno));
        }
        fd_ = -1;
    }
    return Error::OK();
}

} // namespace port
    
} // namespace mai
