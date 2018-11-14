#include "base/base.h"
#include "error.h"

namespace mai {
    
std::string Error::ToString() const {
    std::string s;
    if (filename_) {
        char buf[260];
        snprintf(buf, arraysize(buf), "[%s:%d] ", filename_, fileline_);
        s.append(buf);
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
        default:
            break;
    }
    s.append(message());
    return s;
}
    
} // namespace mai
