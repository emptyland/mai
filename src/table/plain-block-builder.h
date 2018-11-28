#ifndef MAI_TABLE_PLAIN_BLOCK_BUILDER_H_
#define MAI_TABLE_PLAIN_BLOCK_BUILDER_H_

#include "base/allocators.h"
#include "glog/logging.h"
#include <string>
#include <string_view>

namespace mai {
class Comparator;
namespace table {
    
class PlainBlockBuilder final {
public:
    PlainBlockBuilder(const Comparator *ucmp, bool last_level)
        : ucmp_(DCHECK_NOTNULL(ucmp))
        , last_level_(last_level) {}
    
    void Reset() {
        buf_.clear();
        last_key_.clear();
        scope_.Purge();
        finish_ = false;
    }
    
    uint32_t Add(std::string_view key, std::string_view value);
    
    std::string_view Finish() {
        scope_.Purge();
        finish_ = true;
        return buf_;
    }
    
    size_t CurrentSizeEstimate() const { return buf_.size(); }
    
private:
    const Comparator *const ucmp_;
    const bool last_level_;
    std::string buf_;
    std::string last_key_;
    bool finish_ = false;
    base::ScopedMemory scope_;
}; // class PlainBlockBuilder
    
} // namespace table
    
} // namespace mai


#endif // MAI_TABLE_PLAIN_BLOCK_BUILDER_H_
