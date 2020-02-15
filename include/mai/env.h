#ifndef MAI_ENV_H_
#define MAI_ENV_H_

#include "mai/error.h"
#include <string_view>
#include <string>
#include <memory>
#include <vector>

namespace mai {
    
class SequentialFile;
class WritableFile;
class RandomAccessFile;
class Allocator;
class ThreadLocalSlot;
class RandomGenerator;
    
class Env {
public:
    Env();
    virtual ~Env();
    
    static Env *Default();
    static Env *NewChrootEnv(const std::string root, Env *delegated);
    
    virtual Error NewSequentialFile(const std::string &file_name,
                                    std::unique_ptr<SequentialFile> *file,
                                    bool use_mmap = true) = 0;
    
    virtual Error NewWritableFile(const std::string &file_name,
                                  bool append,
                                  std::unique_ptr<WritableFile> *file) = 0;
    
    virtual Error NewRandomAccessFile(const std::string &file_name,
                                      std::unique_ptr<RandomAccessFile> *file,
                                      bool use_mmap = true) = 0;
    
    virtual Error MakeDirectory(const std::string &name,
                                bool create_if_missing) = 0;
    
    virtual Error FileExists(const std::string &file_name) = 0;
    
    virtual Error GetChildren(const std::string &dir_name,
                              std::vector<std::string> *children) = 0;
    
    virtual Error DeleteFile(const std::string &name, bool recursive) = 0;
    
    virtual Error GetFileSize(const std::string &name, uint64_t *size) = 0;

    // New system real random generator
    virtual Error NewRealRandomGenerator(std::unique_ptr<RandomGenerator> *random) = 0;
    
    // Get a OS level memory allocator
    virtual Allocator *GetLowLevelAllocator() = 0;

    virtual std::string GetWorkDirectory() = 0;
    
    virtual std::string GetAbsolutePath(const std::string &file_name) = 0;
    
    virtual uint64_t CurrentTimeMicros();
    
    // TLS
    virtual Error NewThreadLocalSlot(const std::string &name,
                                     void (*dtor)(void *),
                                     std::unique_ptr<ThreadLocalSlot> *slot) = 0;
    
    virtual int GetNumberOfCPUCores() = 0;
    
}; // class Env

class SequentialFile {
public:
    SequentialFile() {}
    virtual ~SequentialFile() {}

    virtual Error Read(size_t n, std::string_view *result,
                       std::string *scratch) = 0;
    virtual Error Skip(size_t n) = 0;
    virtual Error GetFileSize(uint64_t *size) = 0;
}; // class SequentialFile

class WritableFile {
public:
    WritableFile() {}
    virtual ~WritableFile();
    
    virtual Error Append(std::string_view data) = 0;
    virtual Error PositionedAppend(std::string_view data, uint64_t offset) = 0;
    virtual Error Flush() = 0;
    virtual Error Sync() = 0;
    virtual Error GetFileSize(uint64_t *size) = 0;
    virtual Error Truncate(uint64_t size) = 0;
    
}; // class WritableFile

class RandomAccessFile {
public:
    RandomAccessFile() {}
    virtual ~RandomAccessFile();
    
    virtual Error Read(uint64_t offset, size_t n, std::string_view *result,
                       std::string *scratch) = 0;
    
    virtual Error GetFileSize(uint64_t *size) = 0;
}; // class RandomAccessFile

class ThreadLocalSlot {
public:
    ThreadLocalSlot() {};
    virtual ~ThreadLocalSlot();
    
    virtual std::string name() = 0;
    virtual void *Get() = 0;
    virtual void Set(void *value) = 0;
}; // class ThreadLocalSlot

class RandomGenerator {
public:
    RandomGenerator() {}
    virtual ~RandomGenerator() {}

    virtual uint64_t NextU64() = 0;
    virtual uint32_t NextU32() = 0;
    virtual float NextF32() = 0;
    virtual double NextF64() = 0;
    
    uint64_t NextRange64(uint64_t m, uint64_t n) { return NextU64() % (n - m) + m; }
    uint32_t NextRange32(uint32_t m, uint32_t n) { return NextU32() % (n - m) + m; }
}; // class RandomGenerator
    
} // namespace mai

#endif // MAI_ENV_H_
