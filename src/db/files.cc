#include "db/files.h"
#include <ctype.h>

namespace mai {
    
namespace db {
    
const char Files::kLockName[] = "LOCK";
const char Files::kCurrentName[] = "CURRENT";
const char Files::kManifestPrefix[] = "MANIFEST-";
const char Files::kLogPostfix[] = ".log";
const char Files::kSstTablePostfix[] = ".sst";
const char Files::kXmtTablePostfix[] = ".xmt";
const char Files::kS1tTablePostfix[] = ".s1t";
    
static bool IsNumber(const std::string &maybe) {
    if (maybe.empty()) {
        return false;
    }
    for (char c : maybe) {
        if (!::isdigit(c)) {
            return false;
        }
    }
    return true;
}
    
/*static*/ std::string Files::TableFileName(const std::string &db_name,
                                            const std::string &cf_name,
                                            Kind table_kind, uint64_t number) {
    const char *ext = ".ERROR";
    switch (table_kind) {
    case kSST_Table:
        ext = kSstTablePostfix;
        break;
    case kXMT_Table:
        ext = kXmtTablePostfix;
        break;
    case kS1T_Table:
        ext = kS1tTablePostfix;
        break;
    default:
        break;
    }
    char buf[260];
    ::snprintf(buf, arraysize(buf), "%s/%s/%llu%s", db_name.c_str(),
               cf_name.c_str(), number, ext);
    return buf;
}
    
/*static*/ std::tuple<Files::Kind, uint64_t>
    Files::ParseName(const std::string &name) {
    
    std::tuple<Files::Kind, uint64_t> rv = std::make_tuple(kUnknown, -1);
    
    if (name == kLockName) {
        rv = std::make_tuple(kLock, -1);
    } else if (name == kCurrentName) {
        rv = std::make_tuple(kCurrent, -1);
    } else if (name.find(kManifestPrefix) == 0) {
        // MANIFEST-1
        auto buf = name.substr(kManifestPrefixLength);
        if (IsNumber(buf)) {
            rv = std::make_tuple(kManifest, ::atoll(buf.c_str()));
        }
    } else if (name.rfind(kLogPostfix) == (name.length() - kLogPostfixLength)) {
        // 1.log len(5) postfix(4)
        auto buf = name.substr(0, name.length() - kLogPostfixLength);
        if (IsNumber(buf)) {
            rv = std::make_tuple(kLog, ::atoll(buf.c_str()));
        }
    } else if (name.rfind(kSstTablePostfix) ==
               (name.length() - kTablePostfixLength)) {
        // 1.sst len(5) postfix(4)
        auto buf = name.substr(0, name.length() - kTablePostfixLength);
        if (IsNumber(buf)) {
            rv = std::make_tuple(kSST_Table, ::atoll(buf.c_str()));
        }
    } else if (name.rfind(kXmtTablePostfix) ==
              (name.length() - kTablePostfixLength)) {
        // 1.xmt len(5) postfix(4)
        auto buf = name.substr(0, name.length() - kTablePostfixLength);
        if (IsNumber(buf)) {
            rv = std::make_tuple(kXMT_Table, ::atoll(buf.c_str()));
        }
    } else if (name.rfind(kS1tTablePostfix) ==
               (name.length() - kTablePostfixLength)) {
        // 1.s1t len(5) postfix(4)
        auto buf = name.substr(0, name.length() - kTablePostfixLength);
        if (IsNumber(buf)) {
            rv = std::make_tuple(kS1T_Table, ::atoll(buf.c_str()));
        }
    }
    
    return rv;
}
    

} // namespace db
    
} // namespace mai
