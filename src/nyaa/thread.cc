#include "nyaa/thread.h"
#include "nyaa/nyaa-core.h"

namespace mai {
    
namespace nyaa {

NyThread::NyThread(NyaaCore *owns)
    : NyUDO(&UDOFinalizeDtor<NyThread>)
    , owns_(DCHECK_NOTNULL(owns)) {
}

NyThread::~NyThread() {    
}
    
} // namespace nyaa
    
} // namespace mai
