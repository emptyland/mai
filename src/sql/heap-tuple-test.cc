#include "my/types.h"
#include "base/base.h"
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {
    
struct FormColumn;
class Form;

class ColumnDescriptor final {
public:
    
private:
    int index_;
    const Form *origin_table_;
    const FormColumn *origin_;
    std::string table_name_;
    std::string name_;
    my::Protocol::Type type_;
}; // class ColumnDesc


//
class HeapTuple final {
public:
    
    
private:
    ColumnDescriptor *cds_;
    size_t size_;
    uint32_t null_bitmap_[1];
};

} // namespace sql
    
} // namespace mai
