#include "base/slice.h"
#include "base/base.h"
#include "mai/error.h"

namespace mai {
    
std::string Error::ToString() const {
    std::string s;
    if (filename_) {
        int n = 0;
        const char *x = nullptr;
        size_t len = ::strlen(filename_);
        for (x = filename_ + len; x > filename_; --x) {
            if (*x == '/' || *x == '\\') {
                ++n;
            }
            if (n == 2) {
                x++;
                break;
            }
        }
        s = base::Sprintf("[%s:%d] ", x, fileline_);
    }
    
    switch (code()) {
        case kOk:
            return "Ok";
        case kNotFound:
            s.append("Not Found: ");
            break;
        case kCorruption:
            s.append("Corruption: ");
            break;
        case kNotSupported:
            s.append("Not Supported: ");
            break;
        case kInvalidArgument:
            s.append("Invalid Argument: ");
            break;
        case kIOError:
            s.append("IO Error: ");
            break;
        case kEOF:
            s.append("EOF: ");
            break;
        case kBusy:
            s.append("Busy: ");
            break;
        case kTryAgain:
            s.append("Try Again: ");
            break;
        default:
            break;
    }
    s.append(message());
    return s;
}
    
} // namespace mai
