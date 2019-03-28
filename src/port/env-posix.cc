#include "port/file-posix.h"
#include "port/tls-posix.h"
#include "base/io-utils.h"
#include "mai/allocator.h"
#include <chrono>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <unistd.h>

namespace mai {
    
namespace port {
    
class PosixMmapAllocator final : public Allocator {
public:
    PosixMmapAllocator(size_t page_size)
        : page_size_(page_size) {}
    virtual ~PosixMmapAllocator() {}
    
    virtual void *Allocate(size_t size, size_t /*alignment*/) override {
        size_t alloc_size = RoundUp(size, page_size_);
        if (alloc_size == 0) {
            return nullptr;
        }
        void *block = ::mmap(nullptr, alloc_size, PROT_READ|PROT_WRITE,
                             MAP_ANON|MAP_PRIVATE, -1, 0);
        if (block == MAP_FAILED) {
            return nullptr;
        }
        return block;
    }
    
    virtual void Free(const void *chunk, size_t size) override {
        void *block = const_cast<void *>(chunk);
        if (!block) {
            return;
        }
        int rv = ::munmap(block, size);
        if (rv < 0) {
            PLOG(ERROR) << "munmap() fail!";
        }
    }
    
    virtual Error SetAccess(void *chunk, size_t size, int access) override {
        int flags = 0;
        if (access == 0) {
            flags = PROT_NONE;
        } else {
            flags |= (access & kRd) ? PROT_READ  : 0;
            flags |= (access & kWr) ? PROT_WRITE : 0;
            flags |= (access & kEx) ? PROT_EXEC  : 0;
        }
        if (::mprotect(chunk, size, flags) < 0) {
            return MAI_CORRUPTION(strerror(errno));
        }
        return Error::OK();
    }
    
    virtual size_t granularity() override { return page_size_; }
    
private:
    const size_t page_size_;
}; // class PosixMmapAllocator
    
class PosixEnv final : public Env {
public:
    PosixEnv()
        : low_level_alloc_(new PosixMmapAllocator(::getpagesize())) {}

    virtual ~PosixEnv() {}
    
    virtual Error NewSequentialFile(const std::string &file_name,
                                    std::unique_ptr<SequentialFile> *file,
                                    bool use_mem_file) override {
        if (use_mem_file) {
            return MemPosixSequentialFile::Open(file_name, file);
        } else {
            return PosixSequentialFile::Open(file_name, file);
        }
    }
    
    virtual Error NewWritableFile(const std::string &file_name, bool append,
                                  std::unique_ptr<WritableFile> *file) override {
        Error rs = PosixWritableFile::Open(file_name, append, file);
        if (!rs) {
            return rs;
        }
        WritableFile *buffered = new base::BufferedWritableFile(file->release(),
                                                                true);
        file->reset(buffered);
        return Error::OK();
        //return PosixWritableFile::Open(file_name, append, file);
    }
    
    virtual Error NewRandomAccessFile(const std::string &file_name,
                                      std::unique_ptr<RandomAccessFile> *file,
                                      bool use_mem_file) override {
        if (use_mem_file) {
            return MemPosixRandomAccessFile::Open(file_name, file);
        } else {
            return PosixRandomAccessFile::Open(file_name, file);
        }
    }
    
    virtual Error MakeDirectory(const std::string &name,
                                bool create_if_missing) override {
        int rv = ::mkdir(name.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|
                         S_IRGRP|S_IWGRP|S_IXGRP|
                         S_IROTH|S_IXOTH);
        if (rv < 0) {
            if (create_if_missing) {
                return MAI_IO_ERROR(strerror(errno));
            } else if (errno != EEXIST) {
                return MAI_IO_ERROR(strerror(errno));
            }
        }
        return Error::OK();
    }
    
    virtual Error FileExists(const std::string &file_name) override {
        struct stat s;
        if (::stat(file_name.c_str(), &s) < 0) {
            return MAI_IO_ERROR(strerror(errno));
        } else {
            return Error::OK();
        }
    }
    
    virtual Error GetFileSize(const std::string &file_name, uint64_t *size) override {
        struct stat s;
        if (::stat(file_name.c_str(), &s) < 0) {
            return MAI_IO_ERROR(strerror(errno));
        }
        *size = s.st_size;
        return Error::OK();
    }
    
    virtual Error GetChildren(const std::string &dir_name,
                              std::vector<std::string> *children) override {
        DIR *d = ::opendir(dir_name.c_str());
        if (!d) {
            return MAI_IO_ERROR(strerror(errno));
        }
        DCHECK_NOTNULL(children)->clear();
        struct dirent entry, *e;
        int rv = 0;
        while ((rv = ::readdir_r(d, &entry, &e)) == 0 && e != nullptr) {
            // Ignore "." and ".."
            if (e->d_namlen == 1 && e->d_name[0] == '.') {
                continue;
            }
            if (e->d_namlen == 2 && e->d_name[0] == '.' && e->d_name[1] == '.') {
                continue;
            }
            std::string child(e->d_name, e->d_namlen);
            children->push_back(child);
        }
        if (rv != 0) {
            return MAI_IO_ERROR(strerror(errno));
        }
        ::closedir(d);
        return Error::OK();
    }
    
    virtual Error DeleteFile(const std::string &name, bool recursive) override {
        if (!recursive) {
            if (::unlink(name.c_str()) < 0) {
                return MAI_IO_ERROR(strerror(errno));
            }
            return Error::OK();
        }
        
        struct stat s;
        if (::stat(name.c_str(), &s) < 0) {
            return MAI_IO_ERROR(strerror(errno));
        }
        if (s.st_mode & S_IFDIR) { // is dir
            std::vector<std::string> children;
            Error rs = GetChildren(name, &children);
            if (!rs) {
                return rs;
            }
            for (auto child : children) {
                rs = DeleteFile(name + "/" + child, true);
                if (!rs) {
                    return rs;
                }
            }
            if (::rmdir(name.c_str()) < 0) {
                return MAI_IO_ERROR(strerror(errno));
            }
        } else {
            if (::unlink(name.c_str()) < 0) {
                return MAI_IO_ERROR(strerror(errno));
            }
        }
        return Error::OK();
    }
    
    virtual std::string GetWorkDirectory() override {
        char dir[MAXPATHLEN];
        return ::getcwd(dir, arraysize(dir));
    }
    
    virtual std::string GetAbsolutePath(const std::string &file_name) override {
        if (file_name.empty() || file_name[0] == '/') {
            return file_name;
        }
        return GetWorkDirectory() + "/" + file_name;
    }
    
    virtual Allocator *GetLowLevelAllocator() override {
        return low_level_alloc_.get();
    }
    
    virtual Error NewThreadLocalSlot(const std::string &name,
                                     void (*dtor)(void *),
                                     std::unique_ptr<ThreadLocalSlot> *result) override {
        return PosixThreadLocalSlot::New(name, dtor, result);
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(PosixEnv);
private:
    std::unique_ptr<Allocator> low_level_alloc_;
}; // class EnvPosix
    
} // namespace port

/*static*/ Env *Env::Default() {
    static ::mai::port::PosixEnv env;
    return &env;
}
    
} // namespace mai
