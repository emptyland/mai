#include "nyaa/high-level-ir.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/object-factory.h"
#include "nyaa/nyaa-values.h"

namespace mai {
    
namespace nyaa {

namespace hir {
    
const BuiltinFunction BuiltinFunction::kDecls[] = {
#define DEFINE_DECL(id, name, return_ty) {name, Type::k##return_ty},
        DECL_BUILTIN_FUNCTIONS(DEFINE_DECL)
#undef DEFINE_DECL
};

static const CastPriority kCastPriorities[Type::kMaxTypes][Type::kMaxTypes] = {
    /*LHS: Void*/[Type::kVoid] =  {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kNever, Type::kVoid}, // Int
        {CastPriority::kNever, Type::kVoid}, // Long
        {CastPriority::kNever, Type::kVoid}, // Float
        {CastPriority::kNever, Type::kVoid}, // String
        {CastPriority::kNever, Type::kVoid}, // Array
        {CastPriority::kNever, Type::kVoid}, // Map
        {CastPriority::kNever, Type::kVoid}, // UDO
        {CastPriority::kNever, Type::kVoid}, // Closure
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
        {CastPriority::kBoth, Type::kObject}, // UDO
        {CastPriority::kBoth, Type::kObject}, // Closure
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
        {CastPriority::kBoth, Type::kObject}, // UDO
        {CastPriority::kBoth, Type::kObject}, // Closure
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: Float*/[Type::kFloat] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kRHS, Type::kFloat}, // Int
        {CastPriority::kRHS, Type::kFloat}, // Long
        {CastPriority::kKeep, Type::kFloat}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kBoth, Type::kObject}, // UDO
        {CastPriority::kBoth, Type::kObject}, // Closure
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: String*/[Type::kString] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kLHS, Type::kInt}, // Int
        {CastPriority::kLHS, Type::kLong}, // Long
        {CastPriority::kLHS, Type::kFloat}, // Float
        {CastPriority::kKeep, Type::kString}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kBoth, Type::kObject}, // UDO
        {CastPriority::kBoth, Type::kObject}, // Closure
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
        {CastPriority::kBoth, Type::kObject}, // UDO
        {CastPriority::kBoth, Type::kObject}, // Closure
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
        {CastPriority::kBoth, Type::kObject}, // UDO
        {CastPriority::kBoth, Type::kObject}, // Closure
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: UDO*/[Type::kUDO] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kBoth, Type::kObject}, // Int
        {CastPriority::kBoth, Type::kObject}, // Long
        {CastPriority::kBoth, Type::kObject}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kKeep, Type::kUDO}, // UDO
        {CastPriority::kBoth, Type::kObject}, // Closure
        {CastPriority::kLHS, Type::kObject}, // Object
    },
    
    /*LHS: Closure*/[Type::kClosure] = {
        {CastPriority::kNever, Type::kVoid}, // Void
        {CastPriority::kBoth, Type::kObject}, // Int
        {CastPriority::kBoth, Type::kObject}, // Long
        {CastPriority::kBoth, Type::kObject}, // Float
        {CastPriority::kBoth, Type::kObject}, // String
        {CastPriority::kBoth, Type::kObject}, // Array
        {CastPriority::kBoth, Type::kObject}, // Map
        {CastPriority::kBoth, Type::kObject}, // UDO
        {CastPriority::kKeep, Type::kClosure}, // Closure
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
        {CastPriority::kRHS, Type::kObject}, // UDO
        {CastPriority::kRHS, Type::kObject}, // Closure
        {CastPriority::kKeep, Type::kObject}, // Object
    },
};
    
CastPriority GetCastPriority(Type::ID lhs, Type::ID rhs) {
    DCHECK_LT(static_cast<int>(lhs), Type::kMaxTypes);
    DCHECK_LT(static_cast<int>(rhs), Type::kMaxTypes);
    return kCastPriorities[lhs][rhs];
}
    
static const Value::InstID kCastActions[Type::kMaxTypes][Type::kMaxTypes] = {
    // Void, Int, Long, Float, String, Array, Map, UDO, Closure, Object
    [Type::kVoid] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kMaxInsts, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
        Value::kMaxInsts, // Object,
    },
    [Type::kInt] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kIToL, // Long,
        Value::kIToF, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
        Value::kInbox, // Object,
    },
    [Type::kLong] = {
        Value::kMaxInsts, // Void
        Value::kLToI, // Int
        Value::kMaxInsts, // Long,
        Value::kLToF, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
        Value::kInbox, // Object,
    },
    [Type::kFloat] = {
        Value::kMaxInsts, // Void
        Value::kFToI, // Int
        Value::kFToL, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
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
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
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
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
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
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
        Value::kInbox, // Object,
    },
    [Type::kUDO] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kMaxInsts, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
        Value::kInbox, // Object,
    },
    [Type::kObject] = {
        Value::kMaxInsts, // Void
        Value::kMaxInsts, // Int
        Value::kMaxInsts, // Long,
        Value::kMaxInsts, // Float,
        Value::kMaxInsts, // String,
        Value::kMaxInsts, // Array,
        Value::kMaxInsts, // Map,
        Value::kMaxInsts, // UDO,
        Value::kMaxInsts, // Closure,
        Value::kMaxInsts, // Object,
    },
};

Value::InstID GetCastAction(Type::ID dst, Type::ID src) {
    DCHECK_LT(static_cast<int>(dst), Type::kMaxTypes);
    DCHECK_LT(static_cast<int>(src), Type::kMaxTypes);
    return kCastActions[src][dst];
}
    
static const Value::InstID kBinaryTransfer[20][Type::kMaxTypes] = {
    // Void, Int, Long, Float, String, Array, Map, UDO, Closure, Object
#define DEFINE_TRANSFER(op)  { \
        Value::kMaxInsts, \
        Value::kI##op, \
        Value::kL##op, \
        Value::kF##op, \
        Value::kMaxInsts, \
        Value::kMaxInsts, \
        Value::kMaxInsts, \
        Value::kMaxInsts, \
        Value::kMaxInsts, \
        Value::kO##op, \
    }
    /*IAdd*/ DEFINE_TRANSFER(Add),
    /*ISub*/ DEFINE_TRANSFER(Sub),
    /*IMul*/ DEFINE_TRANSFER(Mul),
    /*IDiv*/ DEFINE_TRANSFER(Div),
    /*IMod*/ DEFINE_TRANSFER(Mod),
    /*LAdd*/ DEFINE_TRANSFER(Add),
    /*LSub*/ DEFINE_TRANSFER(Sub),
    /*LMul*/ DEFINE_TRANSFER(Mul),
    /*LDiv*/ DEFINE_TRANSFER(Div),
    /*LMod*/ DEFINE_TRANSFER(Mod),
    /*FAdd*/ DEFINE_TRANSFER(Add),
    /*FSub*/ DEFINE_TRANSFER(Sub),
    /*FMul*/ DEFINE_TRANSFER(Mul),
    /*FDiv*/ DEFINE_TRANSFER(Div),
    /*FMod*/ DEFINE_TRANSFER(Mod),
    /*OAdd*/ DEFINE_TRANSFER(Add),
    /*OSub*/ DEFINE_TRANSFER(Sub),
    /*OMul*/ DEFINE_TRANSFER(Mul),
    /*ODiv*/ DEFINE_TRANSFER(Div),
    /*OMod*/ DEFINE_TRANSFER(Mod),
#undef DEFINE_TRANSFER
};
    
static const Value::InstID kComparatorTranssfer[Type::kMaxTypes] = {
    // Void, Int, Long, Float, String, Array, Map, UDO, Closure, Object
    Value::kMaxInsts,
    Value::kICmp,
    Value::kLCmp,
    Value::kFCmp,
    Value::kMaxInsts,
    Value::kMaxInsts,
    Value::kMaxInsts,
    Value::kMaxInsts,
    Value::kMaxInsts,
    Value::kOCmp,
};
    
Value::InstID TransformComparatorInst(Type::ID ty) {
    DCHECK_GE(static_cast<int>(ty), 0);
    DCHECK_LT(static_cast<int>(ty), Type::kMaxTypes);
    return kComparatorTranssfer[static_cast<int>(ty)];
}
    
Value::InstID TransformBinaryInst(Type::ID ty, Value::InstID src) {
    switch (src) {
    #define DEFINE_CASE(name) case Value::k##name:
            DECL_HIR_BINARY(DEFINE_CASE)
            break;
    #undef DEFINE_CASE
        default:
            DLOG(FATAL) << "Not binary inst!";
            break;
    }
    DCHECK_LT(static_cast<int>(ty), Type::kMaxTypes);
    return kBinaryTransfer[static_cast<int>(src) - Value::kIAdd][ty];
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
    
    for (Value *inst = insts_begin(); inst != insts_end(); inst = inst->next_) {
        fprintf(fp, "    ");
        inst->PrintTo(fp);
        fprintf(fp, "; line = %d\n", inst->line());
    }
}
    
Object *Constant::AsObject(NyaaCore *N) const {
    switch (type()) {
        case Type::kInt:
            return NySmi::New(smi_);
        case Type::kFloat:
            return N->factory()->NewFloat64(f64_);
        case Type::kString:
        case Type::kArray:
        case Type::kMap:
        case Type::kLong:
        case Type::kObject:
            return stub_;
        default:
            DLOG(FATAL) << "Noreached!";
            break;
    }
    return nullptr;
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
    bool ok = false;
    if (old_val == callee_ && new_val->type() == callee_->type()) {
        callee_ = new_val;
        ok = true;
    }
    return Call::ReplaceUse(old_val, new_val) || ok;
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
    
/*virtual*/ void CallBuiltin::PrintOperator(FILE *fp) const {
    const BuiltinFunction *prop = GetBuiltinFunctionProperty(callee());
    ::fprintf(fp, " %s(", prop->name);
    bool enter = true;
    for (int i = 0; i < argc_; ++i) {
        if (!enter) {
            ::fprintf(fp, ", ");
        }
        args_[i]->PrintValue(fp);
        enter = false;
    }
    ::fprintf(fp, ")");
}
    
/*virtual*/ void Branch::PrintOperator(FILE *fp) const {
    fprintf(fp, "br ");
    cond_->PrintValue(fp);
    if (if_true()) {
        fprintf(fp, " then l%d", if_true()->label());
    }
    if (if_false()) {
        fprintf(fp, " else l%d", if_false()->label());
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
    
Alloca::Alloca(Function *top, Type::ID type, int line)
    : Value(top, nullptr, type, line) {
    if (top->entry()->insts_empty() || !top->entry()->insts_head()->IsAlloca()) {
        top->entry()->InsertHead(this);
    } else {
        Value *prev = top->entry()->insts_begin();
        Value *p = prev->next();
        while (p) {
            if (!p->IsAlloca()) {
                top->entry()->InsertValueAfter(prev, this);
                break;
            }
            prev = p;
            p = p->next();
        }
    }
}

} // namespace hir

} // namespace nyaa
    
} // namespace mai
