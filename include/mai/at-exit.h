#pragma once
#ifndef MAI_AT_EXIT_H_
#define MAI_AT_EXIT_H_

#include <mutex>

namespace mai {

class AtExit {
public:
    enum Linker {
        INITIALIZER
    };
    using Callback = void (*)(void *);
    
    explicit AtExit(Linker);
    ~AtExit();

    static AtExit *This();
    
    void Register(Callback callback, void *params);
private:
    struct Hook;
    
    AtExit *prev_ = nullptr;
    Hook *hook_ = nullptr;
    std::mutex mutex_;
}; // class AtExit

} // namespace mai

#endif // MAI_AT_EXIT_H_
