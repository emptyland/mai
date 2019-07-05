#include "nyaa/ast.h"
#include "nyaa/ir-dag.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/function.h"

namespace mai {
    
namespace nyaa {
    
using ValueTable = std::unordered_map<const ast::String *, dag::Value *,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;
    
namespace {

class BlockScope final {
public:
    
private:
    BlockScope *prev_;
    ValueTable values_;
}; // class BlockScope
    
}; // namespace
    
class IRGeneratorVisitor final : public ast::Visitor {
public:
    virtual ~IRGeneratorVisitor() override {}
    
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        target_ = dag::Function::New(arena_);
//        int nparams = !node->params() ? 0 : static_cast<int>(node->params()->size());
//        if (typed_args_.size() < nparams) {
//            nparams = static_cast<int>(typed_args_.size());
//        }
//        for (int i = 0; i < nparams; ++i) {
//            dag::Type::ID ty = BuiltinType2DAG(typed_args_[i]);
//            dag::Value *param = target_->Parameter(ty, node->line());
//            (void)param;
//        }
        
        //original_->upval(0).
        return IVal::Void();
    }
    
    dag::Type::ID BuiltinType2DAG(BuiltinType ty) {
        switch (ty) {
            case kTypeSmi:
            case kTypeInt:
                return dag::Type::kInt;
            case kTypeMap:
                return dag::Type::kMap;
            case kTypeString:
                return dag::Type::kString;
            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
        return dag::Type::kVoid;
    }
private:
    base::Arena *arena_;
    std::vector<BuiltinType> typed_args_;
    dag::Function *target_ = nullptr;
    NyFunction *original_ = nullptr;
}; // class IRGeneratorVisitor
    
} // namespace nyaa
    
} // namespace mai
