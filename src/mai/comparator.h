#ifndef MAI_COMPARATOR_H_
#define MAI_COMPARATOR_H_

#include <string_view>
#include <string>

namespace mai {
    
class Comparator {
public:
    Comparator() {}
    virtual ~Comparator() {}
    
    virtual int Compare(std::string_view lhs, std::string_view rhs) const = 0;
    
    virtual bool Equals(std::string_view lhs, std::string_view rhs) const {
        return Compare(lhs, rhs) == 0;
    }
    
    virtual const char* Name() const = 0;

    virtual void FindShortestSeparator(std::string* start,
                                       std::string_view limit) const = 0;
    
    virtual void FindShortSuccessor(std::string* key) const = 0;
    
    static const Comparator *Bytewise();
}; // class Comparator
    
} // namespace mai

#endif // MAI_COMPARATOR_H_
