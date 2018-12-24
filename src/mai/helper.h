#ifndef MAI_HELPER_H_
#define MAI_HELPER_H_

#include "mai/db.h"
#include <vector>

namespace mai {
    
class ColumnFamilyCollection {
public:
    explicit ColumnFamilyCollection(DB *owns) : owns_(owns) {}

    ~ColumnFamilyCollection() { ReleaseAll(); }
    
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
    
    ColumnFamily *newest() const { return handles_.back(); }
    
    ColumnFamily *GetOrNull(const std::string &name) {
        for (auto cf : handles_) {
            if (cf->name().compare(name) == 0) {
                return cf;
            }
        }
        return nullptr;
    }
    
    void Reset(DB *owns) {
        ReleaseAll();
        owns_ = owns;
    }
    
private:
    DB *owns_;
    std::vector<ColumnFamily *> handles_;
}; // class ColumnFamilyCollection
    
} // namespace mai


#endif // MAI_HELPER_H_
