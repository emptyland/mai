#ifndef MAI_PORT_TLS_POSIX_H_
#define MAI_PORT_TLS_POSIX_H_

#include "mai/env.h"
#include <errno.h>
#include <pthread.h>

namespace mai {
    
namespace port {
    
class PosixThreadLocalSlot final : public ThreadLocalSlot{
public:
    static Error New(const std::string &name,
                     void (*dtor)(void *),
                     std::unique_ptr<ThreadLocalSlot> *result) {
        pthread_key_t tls_key;
        int rv = ::pthread_key_create(&tls_key, dtor);
        if (rv < 0) {
            return MAI_IO_ERROR(strerror(errno));
        }
        result->reset(new PosixThreadLocalSlot(name, tls_key));
        return Error::OK();
    }
    
    virtual ~PosixThreadLocalSlot() { ::pthread_key_delete(tls_key_); }
    
    virtual std::string name() override { return name_; }
    
    virtual void *Get() override {
        return ::pthread_getspecific(tls_key_);
    }

    virtual void Set(void *value) override {
        ::pthread_setspecific(tls_key_, value);
    }
    
    DEF_VAL_GETTER(pthread_key_t, tls_key);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(PosixThreadLocalSlot);
private:
    PosixThreadLocalSlot(const std::string &name, pthread_key_t tls_key)
        : name_(name)
        , tls_key_(tls_key) {
    }
    
    const std::string name_;
    pthread_key_t tls_key_;
}; // class PosixThreadLocalSlot
    
} // namespace port

    
} // namespace mai

#endif // MAI_PORT_TLS_POSIX_H_
