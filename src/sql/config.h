#ifndef MAI_SQL_CONFIG_H_
#define MAI_SQL_CONFIG_H_

#include "base/slice.h"

namespace mai {
    
namespace sql {


class Files final {
    using Slice = ::mai::base::Slice;
public:
    static std::string
    MetaFileName(const std::string &path, uint64_t file_number) {
        return Slice::Sprintf("%s/META-%06" PRIu64 , path.c_str(),
                              file_number);
    }

    static std::string CurrentFileName(const std::string &path) {
        return path + "/CURRENT";
    }
    
    static std::string FrmFileName(const std::string &path,
                                   const std::string &db_name,
                                   const std::string &table_name) {
        std::string name(path);
        return name.append("/").append(db_name).append("/").append(table_name)
                   .append(".frm");
    }
    
    DISALLOW_ALL_CONSTRUCTORS(Files);
}; // class Files

    
} // namespace sql

} // namespace mai



#endif // MAI_SQL_CONFIG_H_
