#include "port/file-posix.h"
#include <chrono>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>

namespace mai {
    
namespace port {
    
class EnvPosix final : public Env {
public:
    EnvPosix() {}
    virtual ~EnvPosix() {}
    
    virtual Error NewSequentialFile(const std::string &file_name,
                                    std::unique_ptr<SequentialFile> *file,
                                    bool use_mem_file) override {
        if (use_mem_file) {
            return MemSequentialFilePosix::Open(file_name, file);
        } else {
            return MAI_NOT_SUPPORTED("TODO:");
        }
    }
    
    virtual Error NewWritableFile(const std::string &file_name, bool append,
                                  std::unique_ptr<WritableFile> *file) override {
        return WritableFilePosix::Open(file_name, append, file);
    }
    
    virtual Error NewRandomAccessFile(const std::string &file_name,
                                      std::unique_ptr<RandomAccessFile> *file,
                                      bool use_mem_file) override {
        if (use_mem_file) {
            return MemRandomAccessFilePosix::Open(file_name, file);
        } else {
            return MAI_NOT_SUPPORTED("TODO:");
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

    DISALLOW_IMPLICIT_CONSTRUCTORS(EnvPosix);
private:
    
}; // class EnvPosix
    
} // namespace port

/*static*/ Env *Env::Default() {
    static ::mai::port::EnvPosix env;
    return &env;
}
    
} // namespace mai
