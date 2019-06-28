#include "nyaa/runtime.h"
#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"
#include "nyaa/nyaa-values.h"
#include "nyaa/object-factory.h"
#include "nyaa/function.h"
#include "nyaa/builtin.h"

namespace mai {

namespace nyaa {

using ThreadTemplate = arch::ObjectTemplate<NyThread>;
using MapTemplate = arch::ObjectTemplate<NyMap>;
using ObjectTemplate = arch::ObjectTemplate<Object>;
using StringTemplate = arch::ObjectTemplate<NyString>;
using NyaaCoreTemplate = arch::ObjectTemplate<NyaaCore>;
    
/*static*/ int Runtime::Object_IsFalseWarp(Object *ob) {
    return ob == Object::kNil ? 1 : ob->IsFalse() ? 1 : 0;
}

/*static*/ int Runtime::Object_IsTrueWarp(Object *ob) {
    return ob == Object::kNil ? 0 : ob->IsTrue() ? 1 : 0;
}
    
/*static*/ Object *Runtime::Object_EQWarp(Object *lhs, Object *rhs, NyaaCore *N) {
    return lhs == Object::kNil || rhs == Object::kNil ? Object::kNil :
           NySmi::New(Object::Equal(lhs, rhs, N));
}
    
/*static*/ Object *Runtime::Object_NEWarp(Object *lhs, Object *rhs, NyaaCore *N) {
    return lhs == Object::kNil || rhs == Object::kNil ? Object::kNil :
           NySmi::New(!Object::Equal(lhs, rhs, N));
}

/*static*/ Object *Runtime::Object_LTWarp(Object *lhs, Object *rhs, NyaaCore *N) {
    return lhs == Object::kNil || rhs == Object::kNil ? Object::kNil :
           NySmi::New(Object::LessThan(lhs, rhs, N));
}

/*static*/ Object *Runtime::Object_LEWarp(Object *lhs, Object *rhs, NyaaCore *N) {
    return lhs == Object::kNil || rhs == Object::kNil ? Object::kNil :
           NySmi::New(Object::LessEqual(lhs, rhs, N));
}

/*static*/ Object *Runtime::Object_GTWarp(Object *lhs, Object *rhs, NyaaCore *N) {
    return lhs == Object::kNil || rhs == Object::kNil ? Object::kNil :
           NySmi::New(Object::LessThan(rhs, lhs, N));
}

/*static*/ Object *Runtime::Object_GEWarp(Object *lhs, Object *rhs, NyaaCore *N) {
    return lhs == Object::kNil || rhs == Object::kNil ? Object::kNil :
           NySmi::New(Object::LessEqual(rhs, lhs, N));
}
    
/*static*/ Object *Runtime::Thread_GetUpVal(NyThread *thd, int slot) {
    return thd->frame_->upval(slot);
}

/*static*/ void Runtime::Thread_SetUpVal(NyThread *thd, Object *val, int up) {
    thd->frame_->SetUpval(up, val, thd->owns_);
}

/*static*/ Object *Runtime::Thread_GetProto(NyThread *thd, int slot) {
    return thd->frame_->proto()->proto_pool()->Get(slot);
}

/*static*/ Object *Runtime::Thread_Closure(NyThread *thd, int slot) {
    NyFunction *proto = NyFunction::Cast(thd->frame_->proto()->proto_pool()->Get(slot));
    DCHECK_NOTNULL(proto);
    NyClosure *closure = thd->owns_->factory()->NewClosure(proto);
    for (int i = 0; i < proto->n_upvals(); ++i) {
        thd->Bind(i, closure, proto->upval(i));
    }
    return closure;
}
    
/*static*/ Address Runtime::NyaaCore_GetRecoverPoint(NyaaCore *N) {
    return N->code_pool()->kEntryTrampoline->entry_address() + N->recover_point_pc();
}
   
/*static*/ NyRunnable *Runtime::NyaaCore_TryMetaFunction(NyaaCore *N, Object *ob, Operator::ID op) {
    NyString *name = nullptr;
    switch (op) {
        case Operator::kAdd:
            name = N->bkz_pool()->kInnerAdd;
            break;
        case Operator::kSub:
            name = N->bkz_pool()->kInnerSub;
            break;
        case Operator::kMul:
            name = N->bkz_pool()->kInnerMul;
            break;
        case Operator::kDiv:
            name = N->bkz_pool()->kInnerDiv;
            break;
        case Operator::kLE:
            name = N->bkz_pool()->kInnerLe;
            break;
        case Operator::kGT:
            name = N->bkz_pool()->kInnerLe;
            break;
        case Operator::kLT:
            name = N->bkz_pool()->kInnerLt;
            break;
        case Operator::kGE:
            name = N->bkz_pool()->kInnerLt;
            break;
        case Operator::kEQ:
            name = N->bkz_pool()->kInnerEq;
            break;
        case Operator::kNE:
            name = N->bkz_pool()->kInnerEq;
            break;
        case Operator::kMod:
            name = N->bkz_pool()->kInnerMod;
            break;
        case Operator::kUnm:
            name = N->bkz_pool()->kInnerUnm;
            break;
        case Operator::kIndex:
            name = N->bkz_pool()->kInnerIndex;
            break;
        case Operator::kNewindex:
            name = N->bkz_pool()->kInnerNewindex;
            break;
        default:
            DLOG(FATAL) << "Noreached: " << op;
            break;
    }
    switch (ob->GetType()) {
        case kTypeMap:
            return NyMap::Cast(ob)->GetValidMetaFunction(name, N);
        case kTypeThread:
        case kTypeUdo:
            return NyUDO::Cast(ob)->GetValidMetaFunction(name, N);
        default:
            break;
    }
    return nullptr;
}
    
/*static*/ NyString *Runtime::NyaaCore_NewUninitializedString(NyaaCore *N, size_t init_size) {
    return N->factory()->NewUninitializedString(init_size);
}
    
/*static*/ void Runtime::Test_PrintNaSt(Address tp, Address bp) {
    DCHECK_LE(tp, bp);
    for (Address i = tp; i < bp; i += kPointerSize) {
        Address p = *reinterpret_cast<Address *>(i);
        printf("<test:%p> %p\n", i, p);
    }
}

/*static*/ Address Runtime::kExternalLinks[kMaxLinks] = {
    ThreadTemplate::MethodAddress(&NyThread::Set),
    ThreadTemplate::MethodAddress(&NyThread::Get),
    reinterpret_cast<Address>(&Thread_GetUpVal),
    reinterpret_cast<Address>(&Thread_SetUpVal),
    reinterpret_cast<Address>(&Thread_GetProto),
    reinterpret_cast<Address>(&Thread_Closure),
    ThreadTemplate::MethodAddress(&NyThread::RuntimePrepareCall),
    ThreadTemplate::MethodAddress(&NyThread::FinializeCall),
    ThreadTemplate::MethodAddress(&NyThread::RuntimeRet),
    ThreadTemplate::MethodAddress(&NyThread::RuntimeNewMap),
    ThreadTemplate::MethodAddress(&NyThread::RuntimePrepareNew),
    ThreadTemplate::MethodAddress(&NyThread::RuntimeSaveNativeStack),
    ThreadTemplate::MethodAddress(&NyThread::CheckStack),
    ThreadTemplate::MethodAddress(&NyThread::RuntimeConcat),
    
    reinterpret_cast<Address>(&NyaaCore_GetRecoverPoint),
    reinterpret_cast<Address>(&NyaaCore_TryMetaFunction),
    reinterpret_cast<Address>(&NyaaCore_NewUninitializedString),
    NyaaCoreTemplate::MethodAddress(&NyaaCore::GarbageCollectionSafepoint),
    
    reinterpret_cast<Address>(&Object_IsFalseWarp),
    reinterpret_cast<Address>(&Object_IsTrueWarp),
    reinterpret_cast<Address>(&Object::Add),
    reinterpret_cast<Address>(&Object::Sub),
    reinterpret_cast<Address>(&Object::Mul),
    reinterpret_cast<Address>(&Object::Div),
    reinterpret_cast<Address>(&Object::Mod),
    reinterpret_cast<Address>(&Object::Shl),
    reinterpret_cast<Address>(&Object::Shr),
    reinterpret_cast<Address>(&Object::BitAnd),
    reinterpret_cast<Address>(&Object::BitOr),
    reinterpret_cast<Address>(&Object::BitXor),
    reinterpret_cast<Address>(&Object_EQWarp),
    reinterpret_cast<Address>(&Object_NEWarp),
    reinterpret_cast<Address>(&Object_LTWarp),
    reinterpret_cast<Address>(&Object_LEWarp),
    reinterpret_cast<Address>(&Object_GTWarp),
    reinterpret_cast<Address>(&Object_GEWarp),
    reinterpret_cast<Address>(&Object::Get),
    reinterpret_cast<Address>(&Object::Put),
    ObjectTemplate::MethodAddress(&Object::ToString),
    
    StringTemplate::MethodAddress(static_cast<NyString *(NyString::*)(const NyString *, NyaaCore*)>(&NyString::Add)),
    StringTemplate::MethodAddress(&NyString::Done),
    
    MapTemplate::MethodAddress(&NyMap::RawGet),
    MapTemplate::MethodAddress(&NyMap::RawPut),
    
    reinterpret_cast<Address>(&Test_PrintNaSt),
};
    
static_assert(arraysize(Runtime::kExternalLinks) == Runtime::kMaxLinks,
              "Incorrect external links size");
    
} // namespace nyaa
    
} // namespace mai
