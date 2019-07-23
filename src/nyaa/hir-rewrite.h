#ifndef MAI_NYAA_HIR_REWRITE_H_
#define MAI_NYAA_HIR_REWRITE_H_

#include "nyaa/high-level-ir.h"
#include "base/base.h"

namespace mai {
    
namespace nyaa {

namespace hir {
    
void RewriteReplacement(Function *target, BasicBlock *begin, BasicBlock *end, Value *old_val,
                        Value *new_val);
    
Value *EmitCastIfNeed(Function *target, BasicBlock *bb, Type::ID type, Value *from, int line);
    
Value *EmitCast(Function *target, BasicBlock *bb, Value::InstID inst, Value *from, int line);
    
Value *EmitBinary(Function *target, BasicBlock *bb, Value::InstID inst, Value *lhs, Value *rhs,
                  int line);
    
} // namespace hir
    
} // namespace nyaa
    
} // namespace mai


#endif // MAI_NYAA_HIR_REWRITE_H_
