#include "lang/object-visitor.h"
#include "lang/value-inl.h"
#include "lang/channel.h"
#include "lang/metadata-space.h"
#include "lang/metadata.h"

namespace mai {
    
namespace lang {

void IterateObject(Any *host, ObjectVisitor *visitor) {
    if (!host) {
        return;
    }

    const Class *clazz = host->is_forward() ? host->forward()->clazz() : host->clazz();
    switch (static_cast<BuiltinType>(clazz->id())) {
        case kType_array: {
            Array<Any *> *object = static_cast<Array<Any *> *>(host);
            object->Iterate(visitor);
        } break;
        case kType_map: {
            AbstractMap *abs = static_cast<AbstractMap *>(host);
            if (abs->value_type()->is_reference()) {
                static_cast<Map<Any*, Any*>*>(abs)->Iterate(visitor);
            } else {
                static_cast<Map<Any*, uintptr_t>*>(abs)->Iterate(visitor);
            }
        } break;
        case kType_map8: {
            AbstractMap *abs = static_cast<AbstractMap *>(host);
            if (abs->value_type()->is_reference()) {
                static_cast<Map<uint8_t, Any*>*>(abs)->Iterate(visitor);
            }
        } break;
        case kType_map16: {
            AbstractMap *abs = static_cast<AbstractMap *>(host);
            if (abs->value_type()->is_reference()) {
                static_cast<Map<uint16_t, Any*>*>(abs)->Iterate(visitor);
            }
        } break;
        case kType_map32: {
            AbstractMap *abs = static_cast<AbstractMap *>(host);
            if (abs->value_type()->is_reference()) {
                static_cast<Map<uint32_t, Any*>*>(abs)->Iterate(visitor);
            }
        } break;
        case kType_map64: {
            AbstractMap *abs = static_cast<AbstractMap *>(host);
            if (abs->value_type()->is_reference()) {
                static_cast<Map<uint64_t, Any*>*>(abs)->Iterate(visitor);
            }
        } break;
        case kType_channel: {
            Channel *object = static_cast<Channel *>(host);
            if (object->data_type()->is_reference() && object->length() > 0) {
                object->Iterate(visitor);
            }
        } break;
        case kType_closure: {
            Closure *object = static_cast<Closure *>(host);
            if (object->captured_var_size() > 0) {
                object->Iterate(visitor);
            }
            if (object->is_mai_function()) {
                Function *fun = object->function();
                const uint32_t spans_size = fun->const_pool_spans_size();
                for (uint32_t i = 0; i < spans_size; i++) {
                    Span32 *span = fun->const_pool() + i;
                    if (fun->TestConstBitmap(i)) {
                        visitor->VisitPointers(object, reinterpret_cast<Any **>(span),
                                               reinterpret_cast<Any **>(span + 1));
                    }
                }
            }
        } break;
        case kType_Code: {
            Kode *object = static_cast<Kode *>(host);
            object->Iterate(visitor);
        } break;
        default:
            if (clazz->is_reference()) {
                Object *object = static_cast<Object *>(host);
                object->Iterate(visitor);
            }
            break;
    }
}

} // namespace lang

} // namespace mai
