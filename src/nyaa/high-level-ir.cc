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
    
const char *Compare::kNames[] = { "eq", "ne", "lt", "le", "gt", "ge", };
    
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
//    if (!in_edges_.empty()) {
//        fprintf(fp, "[in]: ");
//        for (auto edge : in_edges_) {
//            fprintf(fp, "l%d ", edge->label_);
//        }
//        putc('\n', fp);
//    }
//    if (!out_edges_.empty()) {
//        fprintf(fp, "[out]: ");
//        for (auto edge : out_edges_) {
//            fprintf(fp, "l%d ", edge->label_);
//        }
//        putc('\n', fp);
//    }
    
    Value *inst = root_;
    while (inst) {
        fprintf(fp, "    ");
        inst->PrintTo(fp);
        fprintf(fp, "; line = %d\n", inst->line());
        inst = inst->next();
    }
}
    
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
    
/*virtual*/ bool Invoke::ReplaceUse(Value *old_val, Value *new_val) {
    if (old_val == callee_ && new_val->type() == callee_->type()) {
        callee_ = new_val;
        return true;
    }
    int i = GetArgument(old_val);
    if (i < argc_ && new_val->type() == args_[i]->type()) {
        args_[i] = new_val;
        return true;
    }
    return false;
}

/*virtual*/ void Invoke::PrintOperator(FILE *fp) const {
    ::fprintf(fp, "invoke ");
    callee_->PrintValue(fp);
    ::fprintf(fp, "(");
    
    if (nargs_ >= 0) {
        bool enter = true;
        for (int i = 0; i < argc_; ++i) {
            if (!enter) {
                ::fprintf(fp, ", ");
            }
            args_[i]->PrintValue(fp);
            enter = false;
        }
    } else {
        args_[0]->PrintValue(fp);
        ::fprintf(fp, ", ...");
    }
    ::fprintf(fp, ") wanted=%d nargs=%d", wanted_, nargs_);
}
    
/*virtual*/ void Branch::PrintOperator(FILE *fp) const {
    fprintf(fp, "br ");
    cond_->PrintValue(fp);
    if (if_true_) {
        fprintf(fp, " then l%d", if_true_->label());
    }
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
