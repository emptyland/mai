#include "port/file-posix.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <algorithm>

namespace mai {
    
namespace port {
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class MemSequentialFilePosix
////////////////////////////////////////////////////////////////////////////////////////////////////
    
/*virtual*/ MemPosixSequentialFile::~MemPosixSequentialFile() {
    if (static_cast<void *>(mapped_mem_) != MAP_FAILED) {
        if (::munmap(mapped_mem_, file_size_) < 0) {
            LOG(ERROR) << "munmap() fail: " << strerror(errno);
        }
        mapped_mem_ = nullptr;
    }
    
    if (fd_ != -1) {
        if (::close(fd_) < 0) {
            LOG(ERROR) << "close() fail: " << strerror(errno);
        }
        fd_ = -1;
    }
}
    
/*static*/ Error
MemPosixSequentialFile::Open(const std::string &file_name, std::unique_ptr<SequentialFile> *file) {
    int fd = ::open(file_name.c_str(), O_RDONLY);
    if (fd < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    
    struct stat s;
    if (::fstat(fd, &s) < 0) {
        close(fd);
        return MAI_IO_ERROR(strerror(errno));
    }
    if (s.st_size == 0) {
        file->reset(new MemPosixSequentialFile(fd, s.st_size, nullptr));
        return Error::OK();
    }
    
    void *mapped = ::mmap(nullptr, s.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE,
                          fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return MAI_IO_ERROR(strerror(errno));
    }
    
    file->reset(new MemPosixSequentialFile(fd, s.st_size,
                                           static_cast<char *>(mapped)));
    return Error::OK();
}

/*virtual*/
Error MemPosixSequentialFile::Read(size_t n, std::string_view *result, std::string */*scratch*/) {
    size_t old_pos = position_.load(std::memory_order_relaxed);
    if (old_pos >= file_size_) {
        return MAI_EOF("Skip()");
    }
    size_t read_size = n < file_size_ - old_pos ? n : file_size_ - old_pos;
    
    *result = std::string_view(mapped_mem_ + old_pos, read_size);
    
    size_t new_pos = old_pos + read_size;
    size_t e;
    do {
        e = old_pos;
    } while(!position_.compare_exchange_weak(e, new_pos));
    return Error::OK();
}

/*virtual*/ Error MemPosixSequentialFile::Skip(size_t n) {
    size_t old_pos = position_.load(std::memory_order_relaxed);
    if (old_pos >= file_size_) {
        return MAI_EOF("Skip()");
    }
    size_t new_pos = old_pos + n;
    if (new_pos > file_size_) {
        new_pos = file_size_;
    }
    size_t e;
    do {
        e = old_pos;
    } while(!position_.compare_exchange_weak(e, new_pos));
    return Error::OK();
}

/*virtual*/ Error MemPosixSequentialFile::GetFileSize(uint64_t *size) {
    *size = file_size_;
    return Error::OK();
}

    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class PosixSequentialFile
////////////////////////////////////////////////////////////////////////////////////////////////////

/*virtual*/ PosixSequentialFile::~PosixSequentialFile() {
    if (fd_ != -1) {
        if (::close(fd_) < 0) {
            LOG(ERROR) << "close() fail: " << strerror(errno);
        }
        fd_ = -1;
    }
}

/*static*/ Error PosixSequentialFile::Open(const std::string &file_name,
                                           std::unique_ptr<SequentialFile> *file) {
    int fd = ::open(file_name.c_str(), O_RDONLY);
    if (fd < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    file->reset(new PosixSequentialFile(fd));
    return Error::OK();
}

/*virtual*/
Error PosixSequentialFile::Read(size_t n, std::string_view *result, std::string *scratch) {
#if defined(DEBUG) || defined(_DEBUG)
    scratch->resize(n, 0xcc);
#else
    scratch->resize(n);
#endif
    Error rs;
    char *dst = &(*scratch)[0];
    size_t left = n;
    while (left != 0) {
        ssize_t done = read(fd_, dst, left);
        if (done < 0) {
            if (errno == EINTR) {
                continue;
            }
            rs = MAI_IO_ERROR(strerror(errno));
            break;
        } else if (done == 0) {
            rs = MAI_EOF("read()");
            break;
        }
        left -= done;
        dst  += done;
    }
    *result = std::string_view(scratch->data(), n - left);
    if (rs.IsEof() && left < n) {
        rs = Error::OK();
    }
    return rs;
}
    
/*virtual*/ Error PosixSequentialFile::Skip(size_t n) {
    if (::lseek(fd_, n, SEEK_CUR) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    return Error::OK();
}
    
/*virtual*/ Error PosixSequentialFile::GetFileSize(uint64_t *size) {
    struct stat s;
    if (::fstat(fd_, &s) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    *size = s.st_size;
    return Error::OK();
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class WritableFilePosix
////////////////////////////////////////////////////////////////////////////////////////////////////
    
/*virtual*/ PosixWritableFile::~PosixWritableFile() {
    if (::close(fd_) < 0) {
        LOG(ERROR) << "close() fail: " << strerror(errno);
    }
    fd_ = -1;
}

/*static*/ Error
PosixWritableFile::Open(const std::string &file_name, bool append,
                        std::unique_ptr<WritableFile> *file) {
    int flags = O_WRONLY|O_CREAT;
    if (append) {
        flags |= O_APPEND;
    }
    
    int fd = ::open(file_name.c_str(), flags,
                    S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    if (fd < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    file->reset(new PosixWritableFile(fd));
    return Error::OK();
}

/*virtual*/ Error PosixWritableFile::Append(std::string_view data) {
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
    
/*virtual*/ Error PosixWritableFile::PositionedAppend(std::string_view data, uint64_t offset) {
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
    
/*virtual*/ Error PosixWritableFile::Flush() {
    return Error::OK();
}
    
/*virtual*/ Error PosixWritableFile::Sync() {
    if (::fsync(fd_) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    return Error::OK();
}
    
/*virtual*/ Error PosixWritableFile::GetFileSize(uint64_t *size)  {
    struct stat s;
    if (::fstat(fd_, &s) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    *size = s.st_size;
    return Error::OK();
}
    
/*virtual*/ Error PosixWritableFile::Truncate(uint64_t size) {
    if (::ftruncate(fd_, size) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    return Error::OK();
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
/// class MemPosixRandomAccessFile
////////////////////////////////////////////////////////////////////////////////////////////////////

MemPosixRandomAccessFile::~MemPosixRandomAccessFile() {
    if (static_cast<void *>(mapped_mem_) != MAP_FAILED) {
        if (::munmap(mapped_mem_, file_size_) < 0) {
            LOG(ERROR) << "munmap() fail: " << strerror(errno);
        }
        mapped_mem_ = nullptr;
    }
    
    if (fd_ != -1) {
        if (::close(fd_) < 0) {
            LOG(ERROR) << "close() fail: " << strerror(errno);
        }
        fd_ = -1;
    }
}
    
/*static*/ Error
MemPosixRandomAccessFile::Open(const std::string &file_name,
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
    
    file->reset(new MemPosixRandomAccessFile(fd, s.st_size, static_cast<char *>(mapped)));
    return Error::OK();
}

/*virtual*/
Error MemPosixRandomAccessFile::Read(uint64_t offset, size_t n, std::string_view *result,
                                     std::string */*scratch*/) {
    if (offset > file_size_) {
        return MAI_IO_ERROR("offset out of range!");
    }
    n = (file_size_ - offset < n ? file_size_ - offset : n);
    *result = std::string_view(mapped_mem_ + offset, n);
    return Error::OK();
}

/*virtual*/ Error MemPosixRandomAccessFile::GetFileSize(uint64_t *size) {
    *size = file_size_;
    return Error::OK();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// class PosixRandomAccessFile
////////////////////////////////////////////////////////////////////////////////////////////////////

/*virtual*/ PosixRandomAccessFile::~PosixRandomAccessFile() {
    if (fd_ != -1) {
        if (::close(fd_) < 0) {
            LOG(ERROR) << "close() fail: " << strerror(errno);
        }
        fd_ = -1;
    }
}

/*static*/ Error
PosixRandomAccessFile::Open(const std::string &file_name,
                            std::unique_ptr<RandomAccessFile> *file) {
    int fd = ::open(file_name.c_str(), O_RDONLY);
    if (fd < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    file->reset(new PosixRandomAccessFile(fd));
    return Error::OK();
}

/*virtual*/
Error PosixRandomAccessFile::Read(uint64_t offset, size_t n, std::string_view *result,
                                  std::string *scratch) {
#if defined(DEBUG) || defined(_DEBUG)
    scratch->resize(n, 0xcc);
#else
    scratch->resize(n);
#endif
    Error rs;
    char *dst = &(*scratch)[0];
    size_t left = n;
    while (left != 0) {
        ssize_t done = ::pread(fd_, dst, left, offset);
        if (done < 0) {
            if (errno == EINTR) {
                continue;
            }
            rs = MAI_IO_ERROR(strerror(errno));
            break;
        } else if (done == 0) {
            rs = MAI_EOF("read()");
            break;
        }
        left -= done;
        dst  += done;
    }
    *result = std::string_view(scratch->data(), n - left);
    if (rs.IsEof() && !result->empty()) {
        rs = Error::OK();
    }
    return rs;
}
    
/*virtual*/ Error PosixRandomAccessFile::GetFileSize(uint64_t *size) {
    struct stat s;
    if (::fstat(fd_, &s) < 0) {
        return MAI_IO_ERROR(strerror(errno));
    }
    *size = s.st_size;
    return Error::OK();
}

} // namespace port
    
} // namespace mai
