#include "port/file-posix.h"

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
    
    virtual Error NewWritableFile(const std::string &file_name,
                                  std::unique_ptr<WritableFile> *file) override {
        return WritableFilePosix::Open(file_name, file);
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
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(EnvPosix);
private:
    
}; // class EnvPosix
    
} // namespace port

/*static*/ Env *Env::Default() {
    static ::mai::port::EnvPosix env;
    return &env;
}
    
} // namespace mai
