#ifndef MAI_ERROR_H_
#define MAI_ERROR_H_

#include <string_view>
#include <string>

namespace mai {
    
class Error final {
public:
    Error() : Error(nullptr, 0, kOk, "") {}
    
    Error(const Error &other)
        : Error(other.filename_, other.fileline_, other.code(), other.message()) {
    }
    
    Error(Error &&other)
        : filename_(other.filename_)
        , fileline_(other.fileline_)
        , state_(other.state_) {
        other.filename_ = nullptr;
        other.fileline_ = 0;
        other.state_ = nullptr;
    }
    
    ~Error() { delete [] state_; }
    
    static Error OK() { return Error(); }
    
    static Error NotFound(const char *filename, int fileline,
                          std::string_view message = "") {
        return Error(filename, fileline, kNotFound, message);
    }
    
    static Error Corruption(const char *filename, int fileline,
                            std::string_view message = "") {
        return Error(filename, fileline, kCorruption, message);
    }
    
    static Error NotSupported(const char *filename, int fileline,
                              std::string_view message = "") {
        return Error(filename, fileline, kNotSupported, message);
    }
    
    static Error InvalidArgument(const char *filename, int fileline,
                                 std::string_view message = "") {
        return Error(filename, fileline, kInvalidArgument, message);
    }
    
    static Error IOError(const char *filename, int fileline,
                         std::string_view message = "") {
        return Error(filename, fileline, kIOError, message);
    }
    
    static Error Eof(const char *filename, int fileline,
                         std::string_view message = "") {
        return Error(filename, fileline, kEOF, message);
    }
    
    bool operator !() const { return fail(); }
    
    bool ok() const { return code() == kOk; }
    
    bool fail() const { return !ok(); }
    
    bool IsNotFound() const { return code() == kNotFound; }
    
    bool IsCorruption() const { return code() == kCorruption; }
    
    bool IsNotSupported() const { return code() == kNotSupported; }
    
    bool IsInvalidArgument() const { return code() == kInvalidArgument; }
    
    bool IsIOError() const { return code() == kIOError; }
    
    bool IsEof() const { return code() == kEOF; }
    
    bool IsBusy() const { return code() == kBusy; }
    
    std::string_view message() const {
        return ok() ? ""
        : std::string_view(state_ + 8, *reinterpret_cast<const int *>(state_));
    }
    
    std::string ToString() const;
    
    void operator = (const Error other) {
        filename_ = other.filename_;
        fileline_ = other.fileline_;
        delete [] state_;
        state_ = MakeState(other.code(), other.message());
    }
    
//    void operator = (Error &&other) {
//        filename_ = other.filename_;
//        other.filename_ = nullptr;
//        fileline_ = other.fileline_;
//        other.fileline_ = 0;
//        state_ = other.state_;
//        other.state_ = nullptr;
//    }
    
private:
    enum Code: int {
        kOk,
        kNotFound,
        kCorruption,
        kNotSupported,
        kInvalidArgument,
        kIOError,
        kEOF,
        kBusy,
    };
    
    Error(const char *filename, int fileline, Code code, std::string_view message)
        : filename_(filename)
        , fileline_(fileline)
        , state_(MakeState(code, message)) {}
    
    Code code() const {
        return state_ == nullptr ? kOk : *reinterpret_cast<const Code *>(state_ + 4);
    }
    
    static const char *MakeState(Code code, std::string_view message) {
        if (code == kOk) {
            return nullptr;
        }
        size_t len = message.length() + sizeof(code) + sizeof(int);
        char *state = new char[len];
        *reinterpret_cast<int *>(state) = static_cast<int>(message.length());
        *reinterpret_cast<Code *>(state + 4) = code;
        memcpy(state + 8, message.data(), message.length());
        return state;
    }
    
    const char * filename_;
    int          fileline_;
    const char  *state_ = nullptr;
}; // class Error
    
#define MAI_NOT_FOUND(...) ::mai::Error::NotFound(__FILE__, __LINE__, __VA_ARGS__)
#define MAI_CORRUPTION(...) ::mai::Error::Corruption(__FILE__, __LINE__, __VA_ARGS__)
#define MAI_IO_ERROR(...) ::mai::Error::IOError(__FILE__, __LINE__, __VA_ARGS__)
#define MAI_EOF(...) ::mai::Error::Eof(__FILE__, __LINE__, __VA_ARGS__)
#define MAI_NOT_SUPPORTED(...) ::mai::Error::NotSupported(__FILE__, __LINE__, __VA_ARGS__)
    
} // namespace mai

#endif // MAI_ERROR_H_
