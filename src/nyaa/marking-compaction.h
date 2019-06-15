#ifndef MAI_NYAA_MARKING_COMPACTION_H_
#define MAI_NYAA_MARKING_COMPACTION_H_

#include "nyaa/memory.h"
#include "base/base.h"
#include <stack>

namespace mai {
    
namespace nyaa {
    
class MarkingCompaction final : public GarbageCollectionPolicy {
public:
    MarkingCompaction(NyaaCore *core, Heap *heap);
    virtual ~MarkingCompaction() override;
    
    virtual void Run() override;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(MarkingCompaction);
private:
    
}; // MarkingCompaction
    
} // namespace nyaa
    
} // namespace mai

#endif // MAI_NYAA_MARKING_COMPACTION_H_
