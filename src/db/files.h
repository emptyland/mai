#ifndef MAI_DB_FILES_H_
#define MAI_DB_FILES_H_

#include "base/base.h"
#include <stdio.h>
#include <string>

namespace mai {
    
namespace db {

struct Files final {
    enum Kind {
        kUnknown,
        kLog,
        kSST_Table,
        kXMT_Table,
        kS1T_Table,
        kManifest,
        kCurrent,
        kLock,
    };
    
    static const char kLockName[];
    static const char kCurrentName[];
    
    static const char kManifestPrefix[];
    static const int kManifestPrefixLength = 9;
    
    static const char  kLogPostfix[];
    static const int kLogPostfixLength = 4;
    
    static const char  kSstTablePostfix[];
    static const char  kXmtTablePostfix[];
    static const char  kS1tTablePostfix[];
    static const int kTablePostfixLength = 4;
    
    static std::tuple<Kind, uint64_t> ParseName(const std::string &name);
    
    static std::string LogFileName(const std::string &db_name, uint64_t number) {
        char buf[260];
        ::snprintf(buf, arraysize(buf), "%s/%llu.log", db_name.c_str(), number);
        return buf;
    }
    
    static std::string TableFileName(const std::string &db_name,
                                     const std::string &cf_name,
                                     Kind table_kind, uint64_t number);
    
    static std::string TableFileName(const std::string &cf_path,
                                     Kind table_kind, uint64_t number);
    
    // MANIFEST
    static std::string ManifestFileName(const std::string &db_name,
                                        uint64_t number) {
        char buf[260];
        ::snprintf(buf, arraysize(buf), "%s/MANIFEST-%llu", db_name.c_str(),
                   number);
        return buf;
    }
    
    // CURRENT
    static std::string CurrentFileName(const std::string &db_name) {
        char buf[260];
        ::snprintf(buf, arraysize(buf), "%s/CURRENT", db_name.c_str());
        return buf;
    }
    
    static std::string LockFileName(const std::string &db_name) {
        return db_name + "/LOCK";
    }

    DISALLOW_ALL_CONSTRUCTORS(Files);
}; // struct Files
    
} // namespace db
    
} // namespace mai

#endif // MAI_DB_FILES_H_
