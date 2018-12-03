#include "core/memory-table.h"

namespace mai {
    
namespace core {

/*virtual*/ MemoryTable::~MemoryTable() {}

/*virtual*/ bool MemoryTable::KeyExists(std::string_view key,
                                        SequenceNumber version) const {
    std::string value;
    Error rs = Get(key, version, nullptr, &value);
    return rs.ok();
}
    
} // namespace core
    
} // namespace mai
