#ifndef MAI_DB_COMPACTION_H_
#define MAI_DB_COMPACTION_H_

#include "db/version.h"
#include "core/key-boundle.h"
#include "base/base.h"
#include "mai/error.h"
#include "glog/logging.h"
#include <vector>

namespace mai {
class Iterator;
namespace table {
class TableBuilder;
} // namespace table
namespace db {

class ColumnFamilyImpl;
   
class Compaction {
public:
    explicit Compaction(ColumnFamilyImpl *cfd) : cfd_(DCHECK_NOTNULL(cfd)) {}
    virtual ~Compaction() {}
    
    DEF_PTR_GETTER_NOTNULL(ColumnFamilyImpl, cfd);
    DEF_VAL_PROP_RW(int, target_level);
    DEF_VAL_PROP_RW(uint64_t, target_file_number);
    DEF_VAL_PROP_RW(core::SequenceNumber, oldest_sequence_number);
    DEF_VAL_PROP_RW(std::string, compaction_point)
    DEF_VAL_GETTER(std::vector<Iterator *>, original_input);
    DEF_VAL_MUTABLE_GETTER(std::vector<Iterator *>, original_input);
    
    void AddInput(Iterator *iter) { original_input_.push_back(iter); }
    
    virtual Error Run(table::TableBuilder *builder) = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Compaction);
private:
    ColumnFamilyImpl *cfd_;
    int target_level_;
    uint64_t target_file_number_;
    core::SequenceNumber oldest_sequence_number_;
    std::string compaction_point_;
    std::vector<Iterator *> original_input_;
}; // class Compaction

    
struct CompactionContext {
    int level = -1;
    Version *input_version = nullptr;
    VersionPatch version_patch;
    std::vector<base::Handle<FileMetadata>> inputs[2];
}; // struct CompactionContext
    
} // namespace db

} // namespace mai


#endif // MAI_DB_COMPACTION_H_
