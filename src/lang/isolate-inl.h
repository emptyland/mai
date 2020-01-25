#ifndef MAI_LANG_ISOLATE_INL_H_
#define MAI_LANG_ISOLATE_INL_H_

#include "lang/isolate.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

extern Isolate *__isolate;

inline Heap *Isolate::heap() const { return DCHECK_NOTNULL(heap_); }

inline MetadataSpace *Isolate::metadata_space() const { return DCHECK_NOTNULL(metadata_space_); }


} // namespace lang

} // namespace mai

#endif // MAI_LANG_ISOLATE_INL_H_
