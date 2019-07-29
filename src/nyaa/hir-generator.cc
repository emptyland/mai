#include "nyaa/code-gen-utils.h"
#include "nyaa/hir-rewrite.h"
#include "nyaa/high-level-ir.h"
#include "nyaa/ast.h"
#include "nyaa/profiling.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/builtin.h"
#include "nyaa/function.h"
#include "base/arenas.h"
#include <set>

namespace mai {
    
namespace nyaa {
    
using UpValTable = std::unordered_map<const ast::String *, hir::UpvalDesc,
    base::ArenaHash<const ast::String *>,
    base::ArenaEqualTo<const ast::String *>>;
    
class HIRGeneratorVisitor;
    
namespace {
    
class HIRBlockScope;

struct ValueBundle {
    hir::Value *value;
    HIRBlockScope *out_scope;
}; // struct ValueBundle
    
static bool IsPlaceholder(const ast::String *name) {
    return name->size() == 1 && name->data()[0] == '_';
}

static bool IsNotPlaceholder(const ast::String *name) { return !IsPlaceholder(name); }
    
class HIRBlockScope final {
public:
    using ValueTable = std::unordered_map<const ast::String *, ValueBundle,
        base::ArenaHash<const ast::String *>,
        base::ArenaEqualTo<const ast::String *>>;

    using IncomingPathMap = std::unordered_map<
        const ast::String *,
        std::vector<hir::Phi::Path>,
        base::ArenaHash<const ast::String *>,
        base::ArenaEqualTo<const ast::String *>>;
    
    inline HIRBlockScope(HIRBlockScope *prev, bool is_br_edge = false)
        : is_br_edge_(is_br_edge)
        , prev_(prev) {
        DCHECK_NE(this, prev);
    }
    inline ~HIRBlockScope() {}
    
    DEF_PTR_GETTER(HIRBlockScope, prev);
    DEF_VAL_GETTER(bool, is_br_edge);
    DEF_VAL_PROP_RMW(ValueTable, values);
    DEF_PTR_PROP_RW(hir::BasicBlock, loop_retry);
    DEF_PTR_PROP_RW(hir::BasicBlock, loop_exit);
    
    void PutValueNested(const ast::String *name, hir::Value *val, HIRBlockScope *scope);
    
    void PutValue(const ast::String *name, hir::Value *val) {
        DCHECK(!val->IsVoidTy());
        ValueBundle bundle;
        bundle.value = val;
        bundle.out_scope = nullptr;
        values_[name] = bundle;
    }
    
    HIRBlockScope *GetBranchEdge() {
        for (HIRBlockScope *p = this; p != nullptr; p = p->prev_) {
            if (p->is_br_edge_) {
                return p;
            }
        }
        return nullptr;
    }
    
    inline std::tuple<hir::Value*, HIRBlockScope*> GetValueOrNullNested(const ast::String *name);
    
    inline hir::Value *GetValueOrNull(const ast::String *name) const {
        auto iter = values_.find(name);
        return iter == values_.end() ? nullptr : iter->second.value;
    }
    
    inline std::map<hir::Value *, hir::Value *> MergeBranch(HIRBlockScope *br);
    
    static IncomingPathMap
    MergeBranchs(HIRBlockScope *brs[], hir::BasicBlock *bbs[], size_t n_brs);
    
private:
    bool is_br_edge_;
    HIRBlockScope *prev_;
    ValueTable values_;
    hir::BasicBlock *loop_retry_ = nullptr;
    hir::BasicBlock *loop_exit_ = nullptr;
}; // class BlockScope
    
class HIRGeneratorContext : public ast::VisitorContext {
public:
    explicit HIRGeneratorContext(ast::VisitorContext *x = nullptr) {
        if (HIRGeneratorContext *prev = Cast(x)) {
            scope_ = DCHECK_NOTNULL(prev->scope_);
            //bb_    = prev->bb_;
        }
    }
    ~HIRGeneratorContext() {}
    
    DEF_VAL_PROP_RW(int, n_result);
    DEF_VAL_PROP_RW(bool, lval);
    DEF_VAL_PROP_RW(int, lval_offset);
    DEF_VAL_PROP_RW(IVal, rval);
    DEF_PTR_PROP_RW_NOTNULL2(HIRBlockScope, scope);
    //DEF_PTR_PROP_RW(hir::BasicBlock, bb);
    
    static HIRGeneratorContext *Cast(ast::VisitorContext *ctx) {
        return !ctx ? nullptr : down_cast<HIRGeneratorContext>(ctx);
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(HIRGeneratorContext);
private:
    int n_result_ = 0;
    bool lval_ = false;
    int  lval_offset_ = 0;
    IVal rval_ = IVal::Void();
    HIRBlockScope *scope_ = nullptr;
    //hir::BasicBlock *bb_ = nullptr;
}; // CodeGeneratorContext


void HIRBlockScope::PutValueNested(const ast::String *name, hir::Value *val, HIRBlockScope *scope) {
    //DCHECK(!val->IsVoid());
    ValueBundle bundle;
    bundle.value = val;
    bundle.out_scope = (scope == this) ? nullptr : scope;
    if (bundle.out_scope) {
        for (HIRBlockScope *p = this; p; p = p->prev_) {
            if (p->is_br_edge_) {
                p->values_[name] = bundle;
                return;
            }
        }
        bundle.out_scope = nullptr;
        scope->values_[name] = bundle;
    } else {
        values_[name] = bundle;
    }
}
    
    
inline std::tuple<hir::Value*, HIRBlockScope*>
HIRBlockScope::GetValueOrNullNested(const ast::String *name) {
    HIRBlockScope *p = this;
    while (p) {
        auto iter = p->values_.find(name);
        if (iter != p->values_.end()) {
            if (iter->second.out_scope) {
                p = iter->second.out_scope;
            }
            return {iter->second.value, p};
        }
        p = p->prev_;
    }
    return {nullptr, nullptr};
}
    
inline std::map<hir::Value *, hir::Value *> HIRBlockScope::MergeBranch(HIRBlockScope *br) {
    DCHECK(br->is_br_edge_);
    std::map<hir::Value *, hir::Value *> rv;
    for (const auto &pair : br->values_) {
        if (!pair.second.out_scope) {
            continue;
        }
        hir::Value *old_ver = DCHECK_NOTNULL(pair.second.out_scope->GetValueOrNull(pair.first));
        hir::Value *new_ver = pair.second.value;
        rv.insert({old_ver, new_ver});
    }
    return rv;
}
    
/*static*/ HIRBlockScope::IncomingPathMap
HIRBlockScope::MergeBranchs(HIRBlockScope *brs[], hir::BasicBlock *bbs[], size_t n) {
    if (n < 2) {
        return IncomingPathMap();
    }

    std::map<hir::Value *, hir::Value *> versions;
    IncomingPathMap paths;
    for (size_t i = 1; i < n; ++i) {
        HIRBlockScope *br = brs[i];

        for (const auto &pair : br->values_) {
            if (!pair.second.out_scope) {
                continue;
            }
            
            hir::Value *old_ver = DCHECK_NOTNULL(pair.second.out_scope->GetValueOrNull(pair.first));
            hir::Value *new_ver = pair.second.value;
            
            hir::Phi::Path path;
            path.incoming_bb = bbs[i];
            path.incoming_value = new_ver;
            paths[pair.first].push_back(path);

            versions.insert({new_ver, old_ver});
        }
    }

    for (auto &pair : paths) {
        DCHECK(!pair.second.empty());
        if (pair.second.size() == 1) {
            auto iter = versions.find(pair.second[0].incoming_value);
            DCHECK(iter != versions.end());
            
            hir::Phi::Path path;
            path.incoming_bb = bbs[0];
            path.incoming_value = iter->second;
            pair.second.push_back(path);
        }
    }
    return paths;
}

inline static hir::Type::ID ConvType(BuiltinType ty) {
    switch (ty) {
        case kTypeSmi:
        case kTypeInt:
            return hir::Type::kInt;
        case kTypeMap:
            return hir::Type::kMap;
        case kTypeString:
            return hir::Type::kString;
        default:
            return hir::Type::kObject;
    }
    return hir::Type::kVoid;
}
    
}; // namespace
    
struct ConstFoldContext {
    ConstFoldContext(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : N_(N)
        , target_(target)
        , line_(line)
        , trace_id_(trace_id) {}
    
    NyaaCore *N_;
    hir::Function *target_;
    int line_;
    int trace_id_;
};

template<hir::Type::ID T>
struct ConstFolding : public ConstFoldContext {
    hir::Value *Add(hir::Constant *lhs, hir::Constant *rhs) { return nullptr; }
    hir::Value *Sub(hir::Constant *lhs, hir::Constant *rhs) { return nullptr; }
    hir::Value *Mul(hir::Constant *lhs, hir::Constant *rhs) { return nullptr; }
    hir::Value *Div(hir::Constant *lhs, hir::Constant *rhs, bool *ok) { return nullptr; }
    hir::Value *Mod(hir::Constant *lhs, hir::Constant *rhs, bool *ok) { return nullptr; }
    
    ConstFolding(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};
    
template<> struct ConstFolding<hir::Type::kInt> : public ConstFoldContext {
    hir::Value *Add(hir::Constant *lhs, hir::Constant *rhs) {
        int64_t lval = lhs->smi_val(), rval = rhs->smi_val();
        if (((rval > 0) && (lval > (NySmi::kMaxValue - rval))) ||
            ((rval < 0) && (lval < (NySmi::kMinValue - rval)))) {
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return target_->Long(ll->Add(rval, N_), line_);
        }
        return target_->Int(lval + rval, line_);
    }

    hir::Value *Sub(hir::Constant *lhs, hir::Constant *rhs) {
        int64_t lval = lhs->smi_val(), rval = rhs->smi_val();
        if (((rval > 0) && (lval < (NySmi::kMinValue + rval))) ||
            ((rval < 0) && (lval > (NySmi::kMaxValue + rval)))) {
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return target_->Long(ll->Sub(rval, N_), line_);
        }
        return target_->Int(lval - rval, line_);
    }

    hir::Value *Mul(hir::Constant *lhs, hir::Constant *rhs) {
        bool overflow = false;
        int64_t lval = lhs->smi_val(), rval = rhs->smi_val();
        if (lval > 0) {  /* lval is positive */
            if (rval > 0) {  /* lval and rval are positive */
                if (lval > (NySmi::kMaxValue / rval)) {
                    overflow = true;
                }
            } else { /* lval positive, rval nonpositive */
                if (rval < (NySmi::kMinValue / lval)) {
                    overflow = true;
                }
            } /* lval positive, rval nonpositive */
        } else { /* lval is nonpositive */
            if (rval > 0) { /* lval is nonpositive, rval is positive */
                if (lval < (NySmi::kMinValue / rval)) {
                    overflow = true;
                }
            } else { /* lval and rval are nonpositive */
                if ( (lval != 0) && (rval < (NySmi::kMaxValue / lval))) {
                    overflow = true;
                }
            } /* End if lval and rval are nonpositive */
        }
        if (overflow) {
            base::ScopedArena scoped_buf;
            NyInt *ll = NyInt::NewI64(lval, &scoped_buf);
            return target_->Long(ll->Mul(rval, N_), line_);
        }
        return target_->Int(lval * rval, line_);
    }

    inline hir::Value *Div(hir::Constant *lhs, hir::Constant *rhs, bool *ok) {
        if (rhs->smi_val() == 0) {
            *ok = false;
            return nullptr;
        }
        *ok = true;
        return target_->Int(lhs->smi_val() / rhs->smi_val(), line_);
    }

    inline hir::Value *Mod(hir::Constant *lhs, hir::Constant *rhs, bool *ok) {
        if (rhs->smi_val() == 0) {
            *ok = false;
            return nullptr;
        }
        *ok = true;
        return target_->Int(lhs->smi_val() % rhs->smi_val(), line_);
    }
    
    ConstFolding(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
}; // struct ConstFolding<hir::Type::kInt>
    
template<> struct ConstFolding<hir::Type::kLong> : public ConstFoldContext {
    inline hir::Value *Add(hir::Constant *lhs, hir::Constant *rhs) {
        return target_->Long(lhs->long_val()->Add(rhs->long_val(), N_), line_);
    }
    inline hir::Value *Sub(hir::Constant *lhs, hir::Constant *rhs) {
        return target_->Long(lhs->long_val()->Sub(rhs->long_val(), N_), line_);
    }
    inline hir::Value *Mul(hir::Constant *lhs, hir::Constant *rhs) {
        return target_->Long(lhs->long_val()->Mul(rhs->long_val(), N_), line_);
    }
    inline hir::Value *Div(hir::Constant *lhs, hir::Constant *rhs, bool *ok) {
        if (rhs->long_val()->IsZero()) {
            *ok = false;
            return nullptr;
        }
        return target_->Long(lhs->long_val()->Div(rhs->long_val(), N_), line_);
    }
    inline hir::Value *Mod(hir::Constant *lhs, hir::Constant *rhs, bool *ok) {
        if (rhs->long_val()->IsZero()) {
            *ok = false;
            return nullptr;
        }
        return target_->Long(lhs->long_val()->Mod(rhs->long_val(), N_), line_);
    }
    
    ConstFolding(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
}; // struct ConstFolding<hir::Type::kLong>
    
template<> struct ConstFolding<hir::Type::kFloat> : public ConstFoldContext {
    inline hir::Value *Add(hir::Constant *lhs, hir::Constant *rhs) {
        return target_->Float(lhs->float_val() + rhs->float_val(), line_);
    }
    inline hir::Value *Sub(hir::Constant *lhs, hir::Constant *rhs) {
        return target_->Float(lhs->float_val() - rhs->float_val(), line_);
    }
    inline hir::Value *Mul(hir::Constant *lhs, hir::Constant *rhs) {
        return target_->Float(lhs->float_val() * rhs->float_val(), line_);
    }
    inline hir::Value *Div(hir::Constant *lhs, hir::Constant *rhs, bool *ok) {
        return target_->Float(lhs->float_val() / rhs->float_val(), line_);
    }
    inline hir::Value *Mod(hir::Constant *lhs, hir::Constant *rhs, bool *ok) {
        return target_->Float(::fmod(lhs->float_val(), rhs->float_val()), line_);
    }
    
    ConstFolding(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
}; // struct ConstFolding<hir::Type::kLong>

    
template<hir::Type::ID T>
struct BinaryEmiting : public ConstFoldContext {
    
    inline hir::Value *Add(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return nullptr;
    }
    inline hir::Value *Sub(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return nullptr;
    }
    inline hir::Value *Mul(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return nullptr;
    }
    inline hir::Value *Div(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return nullptr;
    }
    inline hir::Value *Mod(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return nullptr;
    }

    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};
    
    
template<> struct BinaryEmiting<hir::Type::kInt> : public ConstFoldContext {
    inline hir::Value *Add(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->IAdd(bb, lhs, rhs, line_);
    }
    inline hir::Value *Sub(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->ISub(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mul(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->IMul(bb, lhs, rhs, line_);
    }
    inline hir::Value *Div(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->IDiv(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mod(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->IMod(bb, lhs, rhs, line_);
    }
    
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};
    
template<> struct BinaryEmiting<hir::Type::kLong> : public ConstFoldContext {
    inline hir::Value *Add(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->LAdd(bb, lhs, rhs, line_);
    }
    inline hir::Value *Sub(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->LSub(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mul(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->LMul(bb, lhs, rhs, line_);
    }
    inline hir::Value *Div(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->LDiv(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mod(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->LMod(bb, lhs, rhs, line_);
    }
    
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};
    
template<> struct BinaryEmiting<hir::Type::kFloat> : public ConstFoldContext {
    inline hir::Value *Add(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->FAdd(bb, lhs, rhs, line_);
    }
    inline hir::Value *Sub(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->FSub(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mul(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->FMul(bb, lhs, rhs, line_);
    }
    inline hir::Value *Div(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->FDiv(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mod(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->FMod(bb, lhs, rhs, line_);
    }
    
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};
    
template<> struct BinaryEmiting<hir::Type::kObject> : public ConstFoldContext {
    inline hir::Value *Add(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->OAdd(bb, lhs, rhs, line_);
    }
    inline hir::Value *Sub(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->OSub(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mul(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->OMul(bb, lhs, rhs, line_);
    }
    inline hir::Value *Div(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->ODiv(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mod(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        return target_->OMod(bb, lhs, rhs, line_);
    }
    
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};
    
template<> struct BinaryEmiting<hir::Type::kString> : public ConstFoldContext {
    inline hir::Value *Add(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        lhs = target_->Inbox(bb, lhs, line_);
        rhs = target_->Inbox(bb, rhs, line_);
        return target_->OAdd(bb, lhs, rhs, line_);
    }
    inline hir::Value *Sub(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        lhs = target_->Inbox(bb, lhs, line_);
        rhs = target_->Inbox(bb, rhs, line_);
        return target_->OSub(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mul(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        lhs = target_->Inbox(bb, lhs, line_);
        rhs = target_->Inbox(bb, rhs, line_);
        return target_->OMul(bb, lhs, rhs, line_);
    }
    inline hir::Value *Div(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        lhs = target_->Inbox(bb, lhs, line_);
        rhs = target_->Inbox(bb, rhs, line_);
        return target_->ODiv(bb, lhs, rhs, line_);
    }
    inline hir::Value *Mod(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        lhs = target_->Inbox(bb, lhs, line_);
        rhs = target_->Inbox(bb, rhs, line_);
        return target_->OMod(bb, lhs, rhs, line_);
    }
    
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};
    
template<> struct BinaryEmiting<hir::Type::kClosure> : public BinaryEmiting<hir::Type::kString> {
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : BinaryEmiting<hir::Type::kString>(N, target, line, trace_id) {}
};
    
template<> struct BinaryEmiting<hir::Type::kArray> : public ConstFoldContext {
    inline hir::Value *Add(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        hir::CallBuiltin *mfn = target_->CallBuiltin(bb, hir::BuiltinFunction::kGetMetaFunction,
                                                     2, line_);
        mfn->SetArgument(0, lhs);
        mfn->SetArgument(1, target_->String(N_->bkz_pool()->kInnerAdd, line_));

        hir::Invoke *val = target_->Invoke(bb, hir::Type::kObject, mfn, 2, 1, line_);
        val->SetArgument(0, lhs);
        val->SetArgument(1, rhs);
        val->set_trace_id(trace_id_);
        return val;
    }
    inline hir::Value *Sub(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        hir::CallBuiltin *mfn = target_->CallBuiltin(bb, hir::BuiltinFunction::kGetMetaFunction,
                                                     2, line_);
        mfn->SetArgument(0, lhs);
        mfn->SetArgument(1, target_->String(N_->bkz_pool()->kInnerSub, line_));
        
        hir::Invoke *val = target_->Invoke(bb, hir::Type::kObject, mfn, 2, 1, line_);
        val->SetArgument(0, lhs);
        val->SetArgument(1, rhs);
        val->set_trace_id(trace_id_);
        return val;
    }
    inline hir::Value *Mul(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        hir::CallBuiltin *mfn = target_->CallBuiltin(bb, hir::BuiltinFunction::kGetMetaFunction,
                                                     2, line_);
        mfn->SetArgument(0, lhs);
        mfn->SetArgument(1, target_->String(N_->bkz_pool()->kInnerMul, line_));
        
        hir::Invoke *val = target_->Invoke(bb, hir::Type::kObject, mfn, 2, 1, line_);
        val->SetArgument(0, lhs);
        val->SetArgument(1, rhs);
        val->set_trace_id(trace_id_);
        return val;
    }
    inline hir::Value *Div(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        hir::CallBuiltin *mfn = target_->CallBuiltin(bb, hir::BuiltinFunction::kGetMetaFunction,
                                                     2, line_);
        mfn->SetArgument(0, lhs);
        mfn->SetArgument(1, target_->String(N_->bkz_pool()->kInnerDiv, line_));
        
        hir::Invoke *val = target_->Invoke(bb, hir::Type::kObject, mfn, 2, 1, line_);
        val->SetArgument(0, lhs);
        val->SetArgument(1, rhs);
        val->set_trace_id(trace_id_);
        return val;
    }
    inline hir::Value *Mod(hir::BasicBlock *bb, hir::Value *lhs, hir::Value *rhs) const {
        hir::CallBuiltin *mfn = target_->CallBuiltin(bb, hir::BuiltinFunction::kGetMetaFunction,
                                                     2, line_);
        mfn->SetArgument(0, lhs);
        mfn->SetArgument(1, target_->String(N_->bkz_pool()->kInnerMod, line_));
        
        hir::Invoke *val = target_->Invoke(bb, hir::Type::kObject, mfn, 2, 1, line_);
        val->SetArgument(0, lhs);
        val->SetArgument(1, rhs);
        val->set_trace_id(trace_id_);
        return val;
    }
    
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : ConstFoldContext(N, target, line, trace_id) {}
};

template<> struct BinaryEmiting<hir::Type::kMap> : public BinaryEmiting<hir::Type::kArray> {
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : BinaryEmiting<hir::Type::kArray>(N, target, line, trace_id) {}
};
    
template<> struct BinaryEmiting<hir::Type::kUDO> : public BinaryEmiting<hir::Type::kArray> {
    BinaryEmiting(NyaaCore *N, hir::Function *target, int line, int trace_id)
        : BinaryEmiting<hir::Type::kArray>(N, target, line, trace_id) {}
};

class HIRGeneratorVisitor final : public ast::Visitor {
public:
    HIRGeneratorVisitor(std::vector<hir::Type::ID> &&args, UpValTable &&upvals,
                        base::Arena *arena, Profiler *profiler, NyaaCore *core)
        : args_(args)
        , upvals_(upvals)
        , arena_(arena)
        , profiler_(profiler)
        , N_(core) {
    }

    virtual ~HIRGeneratorVisitor() override {}
    
    DEF_VAL_GETTER(Error, error);
    DEF_PTR_GETTER(hir::Function, target);
    
#define ACCEPT(subx, ctx) (subx)->Accept(this, (ctx)); if (error_.fail()) return IVal::Void()
    
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        target_ = hir::Function::New(arena_);
        
        HIRBlockScope trunk_scope(nullptr);
        HIRGeneratorContext top_ctx;
        top_ctx.set_scope(&trunk_scope);
        
        const size_t n_params = !node->params() ? 0 : node->params()->size();
        for (int i = 0; i < n_params; ++i) {
            hir::Value *val = nullptr;
            if (i < args_.size()) {
                val = target_->Parameter(args_[i], node->line());
            } else {
                val = target_->Nil(node->line());
            }
            trunk_scope.PutValue(node->params()->at(i), val);
            
        }
        if (node->vargs()) {
            int64_t nvargs = args_.size() - n_params;
            for (int64_t i = 0; i < nvargs; ++i) {
                vargs_.push_back(args_[i]);
            }
        }

        insert_ = target_->NewBB(nullptr);
        target_->set_entry(insert_);
        ACCEPT(node->value(), &top_ctx);
        return IVal::Void();
    }
    
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override {
        if (node->stmts()) {
            for (auto stmt : *node->stmts()) {
                ACCEPT(stmt, x);
            }
        }
        return IVal::Void();
    }
    
    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        if (node->inits()) {
            HIRGeneratorContext rix(x);
            if (node->names()->size() > 1 && node->GetNWanted() < 0) {
                rix.set_n_result(static_cast<int>(node->names()->size()));
                IVal rval = ACCEPT(node->inits()->at(0), &rix);
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    hir::Value *val = rval.node;
                    if (i > 0) {
                        val = target_->BaseOfStack(insert_, rval.node, static_cast<int>(i),
                                                   node->line());
                    }
                    ctx->scope()->PutValue(node->names()->at(i), val);
                }
            } else {
                rix.set_n_result(1);
                
                for (size_t i = 0; i < node->names()->size(); ++i) {
                    if (i < node->inits()->size()) {
                        IVal rval = ACCEPT(node->inits()->at(i), &rix);
                        ctx->scope()->PutValue(node->names()->at(i), rval.node);
                    } else {
                        ctx->scope()->PutValue(node->names()->at(i), target_->Nil(node->line()));
                    }
                }
            }
        } else {
            for (auto name : *node->names()) {
                if (IsNotPlaceholder(name)) {
                    ctx->scope()->PutValue(name, target_->Nil(node->line()));
                }
            }
        }
        return IVal::Void();
    }
    
    virtual IVal VisitAssignment(ast::Assignment *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);

        HIRGeneratorContext rix(ctx);
        if (node->rvals()->size() == 1 && node->GetNWanted() < 0) {
            rix.set_n_result(static_cast<int>(node->lvals()->size()));
            IVal base = ACCEPT(node->rvals()->front(), &rix);
            
            HIRGeneratorContext lix(ctx);
            lix.set_rval(base);
            lix.set_lval(true);
            for (size_t i = 0; i < node->lvals()->size(); ++i) {
                lix.set_lval_offset(static_cast<int>(i));
                ACCEPT(node->lvals()->at(i), &lix);
            }
        } else {
            rix.set_n_result(1);
            size_t len = std::min(node->lvals()->size(), node->rvals()->size());
            
            HIRGeneratorContext lix(ctx);
            for (size_t i = 0; i < len; ++i) {
                IVal val = ACCEPT(node->rvals()->at(i), &rix);
                lix.set_lval(true);
                lix.set_rval(val);
                ACCEPT(node->lvals()->at(i), &lix);
            }
        }
        return IVal::Void();
    }
    
    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override {
        if (IsPlaceholder(node->name())) {
            return IVal::Void();
        }
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        hir::Value *val = nullptr;
        HIRBlockScope *scope = nullptr;
        std::tie(val, scope) = ctx->scope()->GetValueOrNullNested(node->name());
        if (ctx->lval()) {
            if (val) {
                ctx->scope()->PutValueNested(node->name(), ctx->rval().node, scope);
                return IVal::Void();
            }
            if (auto iter = upvals_.find(node->name()); iter != upvals_.end()) {
                target_->StoreUp(insert_, iter->second.slot, ctx->rval().node, node->line());
                return IVal::Void();
            }
            target_->StoreGlobal(insert_, node->name(), ctx->rval().node, node->line());
            return IVal::Void();
        } else {
            if (val) {
                return IVal::HIR(val);
            }
            if (auto iter = upvals_.find(node->name()); iter != upvals_.end()) {
                val = target_->LoadUp(insert_, hir::Type::kObject, iter->second.slot, node->line());
                return IVal::HIR(val);
            }
            val = target_->LoadGlobal(insert_, hir::Type::kObject, node->name(), node->line());
            return IVal::HIR(val);
        }
    }
    
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        hir::Ret *ret = target_->Ret(insert_, node->line());
        if (!node->rets()) {
            ret->set_wanted(0);
            return IVal::Void();
        }

        int nrets = node->GetNRets();
        HIRGeneratorContext ix(x);
        ix.set_n_result(nrets < 0 ? -1 : 1);
        
        IVal first = ACCEPT(node->rets()->at(0), &ix);
        DCHECK_NE(IVal::kVoid, first.kind);
        
        ret->AddRetVal(first.node);
        for (size_t i = 1; i < node->rets()->size(); ++i) {
            IVal ret_val = ACCEPT(node->rets()->at(i), &ix);
            ret->AddRetVal(ret_val.node);
        }
        ret->set_wanted(nrets);
        return IVal::Void();
    }
    
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        HIRGeneratorContext ix(ctx);
        
        std::vector<hir::Value *> operands;
        for (int i = 0; i < node->n_operands(); ++i) {
            IVal operand = ACCEPT(node->operand(i), &ix);
            DCHECK_EQ(IVal::kHIR, operand.kind);
            operands.push_back(operand.node);
        }
        
        hir::Value *val = nullptr;
        switch (node->op()) {
            case Operator::kAdd:
                val = EmitAdd(operands[0], operands[1], node->line(), node->trace_id());
                break;
            case Operator::kSub:
                val = EmitSub(operands[0], operands[1], node->line(), node->trace_id());
                break;
            case Operator::kMul:
                val = EmitMul(operands[0], operands[1], node->line(), node->trace_id());
                break;
            case Operator::kDiv:
                val = EmitDiv(operands[0], operands[1], node->line(), node->trace_id());
                break;
            case Operator::kMod:
                val = EmitMod(operands[0], operands[1], node->line(), node->trace_id());
                break;
            case Operator::kEQ:
                break;
            case Operator::kNE:
                break;
            case Operator::kLT:
                break;
            case Operator::kLE:
                break;
            case Operator::kGT:
                break;
            case Operator::kGE:
                break;
            default:
                DLOG(FATAL) << "Noreached!";
                break;
        }
        return IVal::HIR(DCHECK_NOTNULL(val));
    }
    
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        int32_t nargs = node->GetNArgs();
        HIRGeneratorContext ix(ctx);
        ix.set_n_result(1);
        
        IVal callee = ACCEPT(node->callee(), &ix);
        ix.set_n_result(nargs < 0 ? -1 : 1);
        
        int wanted = ctx->n_result() == 0 ? 1 : ctx->n_result();
        hir::Type::ID ty;
        if (wanted == 0) {
            ty = hir::Type::kVoid;
        } else {
            ty = hir::Type::kObject;
        }
        std::vector<hir::Value *> args;
        for (int i = 0; node->args() && i < node->args()->size(); ++i) {
            IVal arg = ACCEPT(node->args()->at(i), &ix);
            DCHECK_EQ(IVal::kHIR, arg.kind);
            args.push_back(arg.node);
        }
        
        hir::Invoke *invoke = target_->Invoke(insert_, ty, callee.node, nargs, wanted,
                                              node->line());
        for (int i = 0; i < wanted - 1; ++i) {
            invoke->AddRetType(hir::Type::kObject);
        }
        for (int i = 0; i < args.size(); ++i) {
            invoke->SetArgument(i, args[i]);
        }
        invoke->set_trace_id(node->trace_id());
        return IVal::HIR(invoke);
    }
    
    virtual IVal VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) override {
        return IVal::HIR(target_->Nil(node->line()));
    }

    virtual IVal VisitStringLiteral(ast::StringLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kString, node->line());
        NyString *s = N_->factory()->NewString(node->value()->data(), node->value()->size());
        val->set_string_val(s);
        return IVal::HIR(val);
    }

    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kFloat, node->line());
        val->set_float_val(node->value());
        return IVal::HIR(val);
    }

    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kInt, node->line());
        val->set_smi_val(node->value());
        return IVal::HIR(val);
    }

    virtual IVal VisitIntLiteral(ast::IntLiteral *node, ast::VisitorContext *x) override {
        hir::Constant *val = target_->Constant(hir::Type::kLong, node->line());
        val->set_long_val(NyInt::Parse(node->value()->data(), node->value()->size(), N_->factory()));
        return IVal::HIR(val);
    }

    virtual IVal VisitMapInitializer(ast::MapInitializer *node, ast::VisitorContext *x) override {
        // TODO:
        return IVal::Void();
    }
    
    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        
        HIRGeneratorContext ix(ctx);
        ix.set_n_result(1);
        hir::BasicBlock *origin = insert_;

        IVal cmp = ACCEPT(node->cond(), &ix);
        hir::Branch *br = target_->Branch(insert_, cmp.node, node->cond()->line());
        hir::BasicBlock *if_true = target_->NewBB(insert_);
        insert_ = if_true;
        br->set_if_true(if_true);
        
        HIRGeneratorContext if_true_ctx(ctx);
        HIRBlockScope if_true_scope(ctx->scope(), true/*is_br*/);
        if_true_ctx.set_scope(&if_true_scope);
        ACCEPT(node->then_clause(), &if_true_ctx);
        
        hir::BasicBlock *if_false = nullptr;
        HIRBlockScope if_false_scope(ctx->scope(), true/*is_br*/);
        if (node->else_clause()) {
            HIRGeneratorContext if_false_ctx(ctx);
            if_false = target_->NewBB(insert_);
            insert_ = if_false;
            if_false_ctx.set_scope(&if_false_scope);
            
            ACCEPT(node->else_clause(), &if_false_ctx);
            br->set_if_false(if_false);
        }

        // TODO: reuse out basic block.
        hir::BasicBlock *out = target_->NewBB(if_true);
        insert_ = out;
        if (!node->else_clause()) {
            br->set_if_false(out);
        }

        HIRBlockScope::IncomingPathMap paths;
        if (node->else_clause()) {
            HIRBlockScope *brs[] = {ctx->scope(), &if_true_scope, &if_false_scope};
            hir::BasicBlock *bbs[] = {origin, if_true, if_false};
            paths = HIRBlockScope::MergeBranchs(brs, bbs, 3);
        } else {
            HIRBlockScope *brs[] = {ctx->scope(), &if_true_scope};
            hir::BasicBlock *bbs[] = {origin, if_true};
            paths = HIRBlockScope::MergeBranchs(brs, bbs, 2);
    
        }
        for (auto &pair : paths) {
            hir::Type::ID ty = EmitCastIfNeed(&pair.second, node->line());
            hir::Phi *phi = target_->Phi(out, ty, node->line());
            for (auto path : pair.second) {
                phi->AddIncoming(path.incoming_bb, path.incoming_value);
            }
            
            HIRBlockScope *scope = nullptr;
            std::tie(std::ignore, scope) = ctx->scope()->GetValueOrNullNested(pair.first);
            if (HIRBlockScope *edge = ctx->scope()->GetBranchEdge()) {
                edge->PutValueNested(pair.first, phi, scope);
            } else {
                scope->PutValue(pair.first, phi);
            }
        }
        
        target_->NoCondBranch(if_true, out, node->line());
        if (if_false) {
            out->AddInEdge(if_false);
            target_->NoCondBranch(if_false, out, node->line());
        }

        return IVal::Void();
    }

    virtual IVal VisitWhileLoop(ast::WhileLoop *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        HIRGeneratorContext ix(ctx);
        ix.set_n_result(1);
        hir::BasicBlock *origin = insert_;

        HIRBlockScope while_scope(ix.scope(), true/*is_br_edge*/);
        hir::BasicBlock *retry = target_->NewBB(insert_);
        target_->NoCondBranch(insert_, retry, node->line());
        insert_ = retry;
        ix.set_scope(&while_scope);

        IVal cond = ACCEPT(node->cond(), &ix);

        DCHECK_EQ(IVal::kHIR, cond.kind);
        hir::Branch *br = target_->Branch(retry, cond.node, node->cond()->line());
        hir::BasicBlock *body = target_->NewBB(retry);
        br->set_if_true(body);

        hir::BasicBlock *out = target_->NewBB(nullptr, true/*dont_insert*/);
        while_scope.set_loop_exit(out);
        while_scope.set_loop_retry(retry);

        insert_ = body;
        ACCEPT(node->body(), &ix);

        target_->NoCondBranch(insert_, retry, node->end_line());

        std::vector<hir::Phi *> phi_nodes;
        HIRBlockScope *brs[] = {ctx->scope(), &while_scope};
        hir::BasicBlock *bbs[] = {origin, insert_};
        HIRBlockScope::IncomingPathMap paths = HIRBlockScope::MergeBranchs(brs, bbs, 2);
        for (auto &pair : paths) {
            hir::Type::ID ty = EmitCastIfNeed(&pair.second, node->line());
            hir::Phi *phi = target_->Phi(nullptr, ty, node->line());
            phi_nodes.push_back(phi);
            hir::Value *val = nullptr;
            HIRBlockScope *scope = nullptr;
            std::tie(val, scope) = ctx->scope()->GetValueOrNullNested(pair.first);
            hir::RewriteReplacement(N_, target_, retry, insert_, val, phi);

            for (auto path : pair.second) {
                phi->AddIncoming(path.incoming_bb, path.incoming_value);
            }
            if (HIRBlockScope *edge = ctx->scope()->GetBranchEdge()) {
                edge->PutValueNested(pair.first, phi, scope);
            } else {
                scope->PutValue(pair.first, phi);
            }
        }
        for (auto phi : phi_nodes) { retry->InsertHead(phi); }

        out->set_label(target_->NextBBId());
        out->AddInEdge(insert_);
        target_->AddBasicBlock(out);
        br->set_if_false(out);
        insert_ = out;
        return IVal::Void();
    }

    IVal VisitBreak(ast::Break *node, ast::VisitorContext *x) override {
        HIRGeneratorContext *ctx = HIRGeneratorContext::Cast(x);
        HIRBlockScope *p = ctx->scope();
        while (p) {
            if (p->loop_exit()) {
                target_->NoCondBranch(insert_, p->loop_exit(), node->line());
                break;
            }
            p = p->prev();
        }
        return IVal::Void();
    }

    friend class HIRBlockScope;
private:
    hir::Value *EmitAdd(hir::Value *lhs, hir::Value *rhs, int line, int trace_id) {
        hir::Type::ID ty = EmitCastIfNeed(insert_, &lhs, &rhs, line, true/*for_arith*/);
        switch (ty) {
        #define DEFINE_EMITING(name, ...) \
            case hir::Type::k##name: \
                return BinaryEmiting<hir::Type::k##name>(N_, target_, line, trace_id) \
                      .Add(insert_, lhs, rhs);
            DECL_DAG_TYPES(DEFINE_EMITING)
        #undef DEFINE_EMITING
            default:
                break;
        }
        DLOG(FATAL) << "Noreached!";
        return nullptr;
    }
    
    hir::Value *EmitSub(hir::Value *lhs, hir::Value *rhs, int line, int trace_id) {
        hir::Type::ID ty = EmitCastIfNeed(insert_, &lhs, &rhs, line, true/*for_arith*/);
        switch (ty) {
        #define DEFINE_EMITING(name, ...) \
            case hir::Type::k##name: \
                return BinaryEmiting<hir::Type::k##name>(N_, target_, line, trace_id) \
                      .Sub(insert_, lhs, rhs);
            DECL_DAG_TYPES(DEFINE_EMITING)
        #undef DEFINE_EMITING
            default:
                break;
        }
        DLOG(FATAL) << "Noreached!";
        return nullptr;
    }
    
    hir::Value *EmitMul(hir::Value *lhs, hir::Value *rhs, int line, int trace_id) {
        hir::Type::ID ty = EmitCastIfNeed(insert_, &lhs, &rhs, line, true/*for_arith*/);
        switch (ty) {
        #define DEFINE_EMITING(name, ...) \
            case hir::Type::k##name: \
                return BinaryEmiting<hir::Type::k##name>(N_, target_, line, trace_id) \
                      .Mul(insert_, lhs, rhs);
            DECL_DAG_TYPES(DEFINE_EMITING)
        #undef DEFINE_EMITING
            default:
                break;
        }
        DLOG(FATAL) << "Noreached!";
        return nullptr;
    }
    
    hir::Value *EmitDiv(hir::Value *lhs, hir::Value *rhs, int line, int trace_id) {
        hir::Type::ID ty = EmitCastIfNeed(insert_, &lhs, &rhs, line, true/*for_arith*/);
        switch (ty) {
        #define DEFINE_EMITING(name, ...) \
            case hir::Type::k##name: \
                return BinaryEmiting<hir::Type::k##name>(N_, target_, line, trace_id) \
                      .Div(insert_, lhs, rhs);
                DECL_DAG_TYPES(DEFINE_EMITING)
        #undef DEFINE_EMITING
            default:
                break;
        }
        DLOG(FATAL) << "Noreached!";
        return nullptr;
    }
    
    hir::Value *EmitMod(hir::Value *lhs, hir::Value *rhs, int line, int trace_id) {
        hir::Type::ID ty = EmitCastIfNeed(insert_, &lhs, &rhs, line, true/*for_arith*/);
        switch (ty) {
        #define DEFINE_EMITING(name, ...) \
            case hir::Type::k##name: \
                return BinaryEmiting<hir::Type::k##name>(N_, target_, line, trace_id) \
                      .Mod(insert_, lhs, rhs);
                DECL_DAG_TYPES(DEFINE_EMITING)
        #undef DEFINE_EMITING
            default:
                break;
        }
        DLOG(FATAL) << "Noreached!";
        return nullptr;
    }

    hir::Type::ID EmitCastIfNeed(std::vector<hir::Phi::Path> *paths, int line) {
        hir::Type::ID ty = paths->at(0).incoming_value->type();
        for (size_t i = 1; i < paths->size(); ++i) {
            hir::CastPriority prio = hir::GetCastPriority(ty, paths->at(i).incoming_value->type());
            switch (prio.how) {
                case hir::CastPriority::kKeep:
                case hir::CastPriority::kRHS:
                    break;
                case hir::CastPriority::kBoth:
                case hir::CastPriority::kLHS:
                    ty = prio.type;
                    break;
                case hir::CastPriority::kNever:
                default:
                    DLOG(FATAL) << "Noreached!";
                    break;
            }
        }

        for (size_t i = 0; i < paths->size(); ++i) {
            hir::Value *val = paths->at(i).incoming_value;
            hir::BasicBlock *bb = paths->at(i).incoming_bb;
            if (val->type() != ty) {
                hir::Value::InstID inst = hir::GetCastAction(ty, val->type());
                paths->at(i).incoming_value = hir::EmitCast(N_, target_, bb, inst, val, line);
            }
        }
        return ty;
    }

    hir::Type::ID EmitCastIfNeed(hir::BasicBlock *bb, hir::Value **lhs, hir::Value **rhs, int line,
                                 bool for_arith) {
        hir::Value::InstID inst;
        hir::CastPriority prio;
        if (for_arith) {
            prio = hir::GetArithCastPriority((*lhs)->type(), (*rhs)->type());
        } else {
            prio = hir::GetCastPriority((*lhs)->type(), (*rhs)->type());
        }
        switch (prio.how) {
            case hir::CastPriority::kKeep:
                break;
            case hir::CastPriority::kRHS:
                inst = hir::GetCastAction(prio.type, (*rhs)->type());
                *rhs = hir::EmitCast(N_, target_, bb, inst, *rhs, line);
                break;
            case hir::CastPriority::kLHS:
                inst = hir::GetCastAction(prio.type, (*lhs)->type());
                *lhs = hir::EmitCast(N_, target_, bb, inst, *lhs, line);
                break;
            case hir::CastPriority::kBoth:
                inst = hir::GetCastAction(prio.type, (*lhs)->type());
                *lhs = hir::EmitCast(N_, target_, bb, inst, *lhs, line);
                *rhs = hir::EmitCast(N_, target_, bb, inst, *rhs, line);
                break;
            case hir::CastPriority::kNever:
            default:
                DLOG(FATAL) << "Noreached!";
                return hir::Type::kVoid;
        }
        return prio.type;
    }
    
    std::vector<hir::Type::ID> args_;
    UpValTable upvals_;
    base::Arena *arena_;
    Profiler *profiler_;
    NyaaCore *N_;
    Error error_;
    std::vector<hir::Type::ID> vargs_;
    hir::Function *target_ = nullptr;
    hir::BasicBlock *insert_ = nullptr;
}; // class IRGeneratorVisitor

Error HIR_GenerateHIR(const BuiltinType *argv, size_t argc, const UpvalDesc *desc,
                      const BuiltinType *upvals, size_t n_upvals, ast::AstNode *ast,
                      hir::Function **rv, base::Arena *arena, NyaaCore *core) {
    std::vector<hir::Type::ID> args(argc);
    for (size_t i = 0; i < argc; ++i) {
        args[i] = ConvType(argv[i]);
    }
    UpValTable uvs;
    for (int i = 0; i < n_upvals; ++i) {
        const NyString *s = desc[i].name;
        const ast::String *name = ast::String::New(arena, s->bytes(), s->size());

        hir::UpvalDesc ud;
        ud.slot = i;
        ud.type_hint = ConvType(upvals[i]);
        uvs.insert({name, ud});
    }

    HIRGeneratorVisitor visitor(std::move(args), std::move(uvs), arena, core->profiler(), core);
    HIRGeneratorContext ctx;
    DCHECK_NOTNULL(ast)->Accept(&visitor, &ctx);
    if (visitor.error().fail()) {
        return visitor.error();
    }
    *rv = visitor.target();
    return Error::OK();
}

} // namespace nyaa
    
} // namespace mai
