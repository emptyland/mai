#ifndef MAI_DB_WRITE_BATCH_WITH_INDEX_H_
#define MAI_DB_WRITE_BATCH_WITH_INDEX_H_

#include "base/base.h"
#include "mai/write-batch.h"

namespace mai {
    
namespace db {
    
class WriteBatchWithIndex final : public WriteBatch {
public:
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(WriteBatchWithIndex);
}; // class WriteBatchWithIndex
    
} // namespace db
    
} // namespace mai

#endif // MAI_DB_WRITE_BATCH_WITH_INDEX_H_
