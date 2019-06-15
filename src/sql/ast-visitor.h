#ifndef MAI_SQL_AST_VISITOR_H_
#define MAI_SQL_AST_VISITOR_H_

#include "sql/ast.h"
#include "mai/error.h"
#include <stack>

namespace mai {
    
namespace sql {
    
namespace ast {
    
class ErrorBreakListener {
public:
    ErrorBreakListener() {}
    virtual ~ErrorBreakListener() {}

    virtual void OnError(Error err) = 0;
    
    virtual Error latest_error() const = 0;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ErrorBreakListener);
}; // class ErrorBreakListener
    
template<class T>
class VisitorWithStack : public Visitor {
public:
    VisitorWithStack(ErrorBreakListener *listener = nullptr)
        : listener_(listener) {}
    virtual ~VisitorWithStack() { DCHECK(stack_.empty()); }
    
    Error error() const { return DCHECK_NOTNULL(listener_)->latest_error(); }
    
    T Take(T on_error_val) {
        if (stack_.empty()) {
            Raise(MAI_CORRUPTION("No data in stack."));
            return on_error_val;
        }
        return Pop();
    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(VisitorWithStack);
protected:
    void Raise(Error error) {
        DCHECK_NOTNULL(listener_)->OnError(error);
        std::stack<T> purge;
        stack_.swap(purge);
    }
    
    void Push(T value) { stack_.push(value); }
    
    T Pop() {
        DCHECK(!stack_.empty());
        auto top = stack_.top();
        stack_.pop();
        return top;
    }
    
private:
    std::stack<T> stack_;
    ErrorBreakListener *listener_;
}; // class VisitorWithStack
    

class ErrorBreakVisitor : public Visitor
                        , public ErrorBreakListener {
public:
    ErrorBreakVisitor(Visitor *delegated = nullptr, bool ownership = false)
        : delegated_(delegated)
        , ownership_(ownership) {}

    virtual ~ErrorBreakVisitor() {
        if (ownership_) { delete delegated_; }
    }
    
    void set_delegated(Visitor *delegated, bool ownership = false) {
        if (ownership_) {
            delete delegated_;
        }
        delegated_ = delegated;
        ownership_ = ownership;
    }
    
    DEF_VAL_GETTER(Error, error);
    
    virtual Error latest_error() const override { return error(); }
    
#define DECL_METHOD(name) virtual void Visit##name(name *node) override { \
        if (error_.ok()) { \
            delegated_->Visit##name(node); \
        } \
    }
    DEFINE_AST_NODES(DECL_METHOD)
#undef DECL_METHOD
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(ErrorBreakVisitor);
private:
    virtual void OnError(Error err) override { error_ = err; }
    
    Visitor *delegated_;
    bool ownership_;
    Error error_;
};
    
} // namespace ast
    
} // namespace sql
    
} // namespace mai



#endif // MAI_SQL_AST_VISITOR_H_
