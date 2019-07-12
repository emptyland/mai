#include "nyaa/high-level-ir.h"

namespace mai {
    
namespace nyaa {

namespace hir {

static const CastPriority kCastPriorities[Type::kMaxTypes][Type::kMaxTypes] = {
    /*LHS: Void*/[Type::kVoid] =  {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kNever, Type::kVoid}, // Int
        {CastPriority::kNever, Type::kVoid}, // Long
        {CastPriority::kNever, Type::kVoid}, // Float
        {CastPriority::kNever, Type::kVoid}, // String
        {CastPriority::kNever, Type::kVoid}, // Array
        {CastPriority::kNever, Type::kVoid}, // Map
        {CastPriority::kNever, Type::kVoid}, // Object
    },
    
    /*LHS: Int*/[Type::kInt] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kKeep, Type::kInt}, // Int
        {CastPriority::kLHS, Type::kLong}, // Long
        {CastPriority::kLHS, Type::kFloat}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: Long*/[Type::kLong] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kRHS, Type::kLong}, // Int
        {CastPriority::kKeep, Type::kLong}, // Long
        {CastPriority::kLHS, Type::kFloat}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: Float*/[Type::kFloat] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kRHS, Type::kFloat}, // Int
        {CastPriority::kRHS, Type::kFloat}, // Long
        {CastPriority::kRHS, Type::kFloat}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: String*/[Type::kString] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kBoth, Type::kObject}, // Int
        {CastPriority::kBoth, Type::kObject}, // Long
        {CastPriority::kBoth, Type::kObject}, // Float
        {CastPriority::kKeep, Type::kString}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: Array*/[Type::kArray] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kBoth, Type::kObject}, // Int
        {CastPriority::kBoth, Type::kObject}, // Long
        {CastPriority::kBoth, Type::kObject}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kKeep, Type::kArray}, // Array
        {CastPriority::kLHS, Type::kMap}, // Map
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: Map*/[Type::kMap] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kBoth, Type::kObject}, // Int
        {CastPriority::kBoth, Type::kObject}, // Long
        {CastPriority::kBoth, Type::kObject}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kRHS, Type::kMap}, // Array
        {CastPriority::kKeep, Type::kMap}, // Map
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: Object*/[Type::kObject] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kRHS, Type::kObject}, // Int
        {CastPriority::kRHS, Type::kObject}, // Long
        {CastPriority::kRHS, Type::kObject}, // Float
        {CastPriority::kRHS, Type::kObject}, // String
        {CastPriority::kRHS, Type::kObject}, // Array
        {CastPriority::kRHS, Type::kObject}, // Map
        {CastPriority::kKeep, Type::kObject}, // Object
    },
};
    
CastPriority GetCastPriority(Type::ID lhs, Type::ID rhs) {
    DCHECK_LT(static_cast<int>(lhs), Type::kMaxTypes);
    DCHECK_LT(static_cast<int>(rhs), Type::kMaxTypes);
    return kCastPriorities[lhs][rhs];
}
    
static const Value::Kind kCastActions[Value::kMaxInsts][Value::kMaxInsts] = {
    // Void, Int, Long, Float, String, Array, Map, Object
    [Type::kVoid] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kMaxInsts, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kMaxInsts, // Object,
    },
    [Type::kInt] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kIntToLong, // Long,
        Value::kIntToLong, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kInbox, // Object,
    },
    [Type::kLong] = {
        Value::kMaxInsts, // Void
        Value::kLongToInt, // Int
        Value::kMaxInsts, // Long,
        Value::kLongToFloat, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kInbox, // Object,
    },
    [Type::kFloat] = {
        Value::kMaxInsts, // Void
        Value::kFloatToInt, // Int
        Value::kFloatToLong, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kInbox, // Object,
    },
    [Type::kString] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kMaxInsts, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kInbox, // Object,
    },
    [Type::kArray] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kMaxInsts, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kInbox, // Map,
        Value::kInbox, // Object,
    },
    [Type::kMap] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kMaxInsts, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kInbox, // Array,
        Value::kMaxInsts, // Map,
        Value::kInbox, // Object,
    },
    [Type::kObject] = {
        Value::kMaxInsts, // Void
        Value::kInbox, // Int
        Value::kInbox, // Long,
        Value::kInbox, // Float,
        Value::kInbox, // String,
        Value::kInbox, // Array,
        Value::kInbox, // Map,
        Value::kMaxInsts, // Object,
    },
};

Value::Kind GetCastAction(Type::ID dst, Type::ID src) {
    DCHECK_LT(static_cast<int>(dst), Value::kMaxInsts);
    DCHECK_LT(static_cast<int>(src), Value::kMaxInsts);
    return kCastActions[src][dst];
}

const char *Type::kNames[] = {
#define DEFINE_NAME(name, literal) literal,
    DECL_DAG_TYPES(DEFINE_NAME)
#undef DEFINE_NAME
};
    
void Function::PrintTo(std::string *buf, size_t limit) const {
    buf->resize(limit);
    FILE *fp = ::fmemopen(&(*buf)[0], limit, "w");
    if (!fp) {
        return;
    }
    PrintTo(fp);
    buf->resize(::ftell(fp));
    ::fclose(fp);
}
    
void BasicBlock::PrintTo(FILE *fp) const {
    fprintf(fp, "l%d:\n", label_);
    if (!in_edges_.empty()) {
        fprintf(fp, "[in]: ");
        for (auto edge : in_edges_) {
            fprintf(fp, "l%d ", edge->label_);
        }
        putc('\n', fp);
    }
    if (!out_edges_.empty()) {
        fprintf(fp, "[out]: ");
        for (auto edge : out_edges_) {
            fprintf(fp, "l%d ", edge->label_);
        }
        putc('\n', fp);
    }
    
    Value *inst = root_;
    while (inst) {
        fprintf(fp, "    ");
        inst->PrintTo(fp);
        fprintf(fp, "; line = %d\n", inst->line());
        inst = inst->next();
    }
}
    
//bool Value::ReplaceUse(Value *old_val, Value *new_val) {
//    switch (kind()) {
//#define DEF_CASE(name) case k##name:
//        DECL_HIR_BINARY(DEF_CASE) {
//            BinaryInst *inst = static_cast<BinaryInst *>(this);
//            if (inst->lhs() == old_val && inst->lhs()->type() == new_val->type()) {
//                inst->set_lhs(new_val);
//                return true;
//            }
//            if (inst->rhs() == old_val && inst->rhs()->type() == new_val->type()) {
//                return true;
//            }
//        } break;
//            
//        DECL_HIR_UNARY(DEF_CASE) {
//            UnaryInst *inst = static_cast<UnaryInst *>(this);
//            if (inst->operand() == old_val && inst->operand()->type() == new_val->type()) {
//                inst->set_operand(new_val);
//                return true;
//            }
//        } break;
//            
//        DECL_HIR_CAST(DEF_CASE) {
//            CastInst *inst = static_cast<CastInst *>(this);
//            if (inst->from() == old_val && inst->from()->type() == new_val->type()) {
//                inst->set_from(new_val);
//                return true;
//            }
//        } break;
//            
//        DECL_HIR_STORE(DEF_CASE) {
//            StoreInst *inst = static_cast<StoreInst *>(this);
//            if (inst->src() == old_val && inst->src()->type() == new_val->type()) {
//                inst->set_src(new_val);
//                return true;
//            }
//        } break;
//            
//        case kRet: {
//            Ret *inst = static_cast<Ret *>(this);
//            bool ok = false;
//            for (size_t i = 0; i < inst->ret_vals_size(); ++i) {
//                if (inst->ret_val(i) == old_val) {
//                    inst->set_ret_val(i, new_val);
//                    ok = true;
//                }
//            }
//            return ok;
//        } break;
//            
//        // TODO:
//          
//#undef DEF_CASE
//        default:
//            DLOG(FATAL) << "Noreached!";
//            break;
//    }
//    return false;
//}
    
/*virtual*/ void Constant::PrintOperator(FILE *fp) const {
    ::fprintf(fp, "%s ", Type::kNames[type()]);
    switch (type()) {
        case Type::kInt:
            ::fprintf(fp, "%lld", smi_);
            break;
        case Type::kFloat:
            ::fprintf(fp, "%f", f64_);
            break;
        case Type::kString:
            ::fprintf(fp, "\'%s\'", str_->data());
            break;
        case Type::kObject:
            ::fprintf(fp, "nil");
            break;
        default:
            DLOG(FATAL) << "Noreached!" << Type::kNames[type()];
            break;
    }
}
    
/*virtual*/ bool BinaryInst::ReplaceUse(Value *old_val, Value *new_val) {
    bool ok = false;
    if (old_val == lhs_ && new_val->type() == lhs_->type()) {
        lhs_ = new_val;
        ok = true;
    }
    if (old_val == rhs_ && new_val->type() == rhs_->type()) {
        rhs_ = new_val;
        ok = true;
    }
    return ok;
}
    
/*virtual*/ void Branch::PrintOperator(FILE *fp) const {
    fprintf(fp, "br ");
    cond_->PrintValue(fp);
    fprintf(fp, " then l%d", if_true_->label());
    if (if_false_) {
        fprintf(fp, " else l%d", if_false_->label());
    }
}
    
/*virtual*/ bool Phi::ReplaceUse(Value *old_val, Value *new_val) {
    bool ok = false;
    for (size_t i = 0; i < incoming_.size(); ++i) {
        if (incoming_[i].incoming_value == old_val &&
            incoming_[i].incoming_value->type() == new_val->type()) {
            incoming_[i].incoming_value = new_val;
            ok = true;
        }
    }
    return ok;
}
    
/*virtual*/ void Phi::PrintOperator(FILE *fp) const {
    fprintf(fp, "phi ");
    for (const auto &path : incoming_) {
        fprintf(fp, "[l%d ", path.incoming_bb->label());
        path.incoming_value->PrintValue(fp);
        fprintf(fp, "] ");
    }
}
    
/*virtual*/ bool Ret::ReplaceUse(Value *old_val, Value *new_val) {
    bool ok = false;
    for (size_t i = 0; i < ret_vals_size(); ++i) {
        if (ret_val(i) == old_val) {
            set_ret_val(i, new_val);
            ok = true;
        }
    }
    return ok;
}

/*virtual*/ void Ret::PrintOperator(FILE *fp) const {
    ::fprintf(fp, "ret(%d) ", wanted_);
    bool enter = false;
    for (auto val : ret_vals_) {
        if (enter) {
            ::fprintf(fp, ", ");
        }
        val->PrintValue(fp);
        enter = true;
    }
}

} // namespace hir

} // namespace nyaa
    
} // namespace mai
