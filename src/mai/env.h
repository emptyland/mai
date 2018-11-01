#ifndef MAI_ENV_H_
#define MAI_ENV_H_

#include <string_view>
#include <string>
#include <memory>
#include "mai/error.h"

namespace mai {
    
class WritableFile;
class RandomAccessFile;
    
class Env {
public:
    Env();
    virtual ~Env();
    
    static Env *Default();
    
    virtual Error NewWritableFile(const std::string &file_name,
                                  std::unique_ptr<WritableFile> *file) = 0;
    
    virtual Error NewRandomAccessFile(const std::string &file_name,
                                      std::unique_ptr<RandomAccessFile> *file,
                                      bool mem_file = true) = 0;
    
}; // class Env
    
class WritableFile {
public:
    WritableFile() {}
    virtual ~WritableFile();
    
    virtual Error Append(std::string_view data) = 0;
    virtual Error PositionedAppend(std::string_view data, uint64_t offset) = 0;
    virtual Error Close() = 0;
    virtual Error Flush() = 0;
    virtual Error Sync() = 0;
    
}; // class WritableFile
    
class RandomAccessFile {
public:
    RandomAccessFile() {}
    virtual ~RandomAccessFile();
    
    virtual Error Read(uint64_t offset, size_t n, std::string_view *result,
                       std::string *scratch) = 0;
    
    virtual Error GetFileSize(uint64_t *size) = 0;
    
    virtual Error Close() = 0;
}; // class RandomAccessFile
    
} // namespace mai

#endif // MAI_ENV_H_
