#ifndef MAI_SQL_AST_NODES_H_
#define MAI_SQL_AST_NODES_H_

#include "base/arena-utils.h"
#include "base/base.h"

namespace mai {
    
namespace sql {
    
#define DEFINE_AST_NODES(V) \
    DEFINE_DDL_NODES(V) \
    DEFINE_DML_NODES(V)
    
#define DEFINE_DDL_NODES(V) \
    V(CreateTable) \
    V(DropTable)
    
#define DEFINE_DML_NODES(V) \
    V(Query)
    
class AstVisitor;
class AstFactory;
    
#define PRE_DECLARE_NODE(name) class name;
DEFINE_AST_NODES(PRE_DECLARE_NODE)
#undef PRE_DECLARE_NODE
    
class AstNode {
public:
    enum Kind {
#define DECL_ENUM(name) k##name,
        DEFINE_AST_NODES(DECL_ENUM)
#undef  DECL_ENUM
    }; // enum Kind
    
    virtual void Accept(AstVisitor *v) = 0;
    virtual Kind kind() const = 0;
    virtual bool is_ddl() const { return false; }
    virtual bool is_dml() const { return false; }
    virtual bool is_command() const { return false; }
    
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }
    
    void operator delete (void *p) = delete;
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(AstNode);
protected:
    AstNode() {}
}; // class AstNode
    
#define DEF_AST_NODE(clazz) \
    virtual void Accept(AstVisitor *v) override { v->Visit##clazz(this); } \
    virtual Kind kind() const override { return k##clazz; } \
    static const clazz *Cast(const AstNode *n) { \
        return n->kind() == k##clazz ? down_cast<const clazz>(n) : nullptr; \
    } \
    static clazz *Cast(AstNode *n) { \
        return n->kind() == k##clazz ? down_cast<clazz>(n) : nullptr; \
    } \
    friend class AstFactory;

using AstString = base::ArenaString;

////////////////////////////////////////////////////////////////////////////////
/// class AstVisitor
////////////////////////////////////////////////////////////////////////////////

class AstVisitor {
public:
    AstVisitor() {}
    virtual ~AstVisitor() {}
    
#define DECL_METHOD(name) virtual void Visit##name(name *node) {}
    DEFINE_AST_NODES(DECL_METHOD)
#undef DECL_METHOD
}; // class AstVisitor
    
////////////////////////////////////////////////////////////////////////////////
/// DDL
////////////////////////////////////////////////////////////////////////////////

class DDLStatement : public AstNode {
public:
    virtual bool is_ddl() const override { return true; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DDLStatement);
protected:
    DDLStatement() {}
}; // public DDLStatement
    
    
class CreateTable final : public DDLStatement {
public:
    virtual bool is_command() const override { return true; }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, schema_name);
    
    DEF_AST_NODE(CreateTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(CreateTable);
private:
    CreateTable(const AstString *schema_name)
        : schema_name_(DCHECK_NOTNULL(schema_name)) {}

    const AstString *schema_name_;
}; // class CreateTable

    
////////////////////////////////////////////////////////////////////////////////
/// DML
////////////////////////////////////////////////////////////////////////////////
    
class DMLStatement : public AstNode {
public:
    virtual bool is_dml() const { return true; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DMLStatement);
};
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_AST_NODES_H_
