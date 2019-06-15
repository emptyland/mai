#ifndef MAI_HELPER_H_
#define MAI_HELPER_H_

#include "mai/db.h"
#include <vector>

namespace mai {
    
class ColumnFamilyCollection {
public:
    explicit ColumnFamilyCollection(DB *owns, bool ownership = false)
        : owns_(owns)
        , ownership_(ownership) {}

    ~ColumnFamilyCollection() {
        ReleaseAll();
        if (ownership_) {
            delete owns_;
        }
    }

    DB *owns() const { return owns_; }
    
    ColumnFamily *newest() const { return handles_.back(); }
    
    void ReleaseAll() {
        for (auto cf : handles_) {
            owns_->ReleaseColumnFamily(cf);
        }
        handles_.clear();
    }
    
    std::vector<ColumnFamily *> *ReceiveAll() {
        ReleaseAll();
        return &handles_;
    }
    
    ColumnFamily **Receive() {
        handles_.push_back(nullptr);
        return &handles_.back();
    }
    
    ColumnFamily *GetOrNull(const std::string &name) {
        for (auto cf : handles_) {
            if (cf->name().compare(name) == 0) {
                return cf;
            }
        }
        return nullptr;
    }
    
    void Attach(DB *owns, bool ownership = false) {
        ReleaseAll();
        if (ownership_) {
            delete owns_;
        }
        owns_ = owns;
        ownership_ = ownership;
    }
    
private:
    DB *owns_;
    bool ownership_;
    std::vector<ColumnFamily *> handles_;
}; // class ColumnFamilyCollection
    
} // namespace mai


#endif // MAI_HELPER_H_
