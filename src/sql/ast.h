#ifndef MAI_SQL_AST_NODES_H_
#define MAI_SQL_AST_NODES_H_

#include "base/arena-utils.h"
#include "base/base.h"

namespace mai {
    
namespace sql {
    
#define DEFINE_AST_NODES(V) \
    DEFINE_DDL_NODES(V) \
    DEFINE_DML_NODES(V) \
    DEFINE_TCL_NODES(V) \
    DEFINE_UTIL_NODES(V)
    
#define DEFINE_DDL_NODES(V) \
    V(CreateTable) \
    V(DropTable)
    
#define DEFINE_DML_NODES(V) \
    V(Query)
    
#define DEFINE_UTIL_NODES(V) \
    V(ShowTables)
    
#define DEFINE_TCL_NODES(V) V(TCLStatement)
    
class AstVisitor;
class AstFactory;
    
#define PRE_DECLARE_NODE(name) class name;
DEFINE_AST_NODES(PRE_DECLARE_NODE)
#undef PRE_DECLARE_NODE
    
class Block;
class Statement;
    
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
    virtual bool is_tcl() const { return false; }
    virtual bool is_util() const { return false; }
    virtual bool is_statement() const { return false; }
    virtual bool is_command() const { return false; }
    
#define DECL_TESTER(name) bool Is##name() const { return kind() == k##name; }
    DEFINE_AST_NODES(DECL_TESTER)
#undef  DECL_TESTER
    
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
/// Block
////////////////////////////////////////////////////////////////////////////////
    
class Block final {
public:
    using Stmts = base::ArenaVector<Statement *>;
    
    inline void AddStmt(Statement *stmt);
    
    size_t stmts_size() const { return stmts_.size(); }
    Statement *stmt(size_t i) const { return stmts_[i]; }
    Stmts::const_iterator begin() const { return stmts_.begin(); }
    Stmts::const_iterator end() const { return stmts_.end(); }
    
    void *operator new (size_t size, base::Arena *arena) {
        return arena->Allocate(size);
    }
    
    void operator delete (void *p) = delete;
    
    friend class AstFactory;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Block);
private:
    Block(base::Arena *arena)
        : stmts_(arena) {
    }
    
    Stmts stmts_;
}; // class Block
    
class Statement : public AstNode {
public:
    virtual bool is_statement() const override { return true; }

    DEF_PTR_PROP_RW_NOTNULL2(Block, owns);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Statement);
protected:
    Statement() {}
    Block *owns_;
}; // class Statement
    
inline void Block::AddStmt(Statement *stmt) {
    stmts_.push_back(stmt);
    stmt->set_owns(this);
}
    
////////////////////////////////////////////////////////////////////////////////
/// DDL
////////////////////////////////////////////////////////////////////////////////

class DDLStatement : public Statement {
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
    

class DropTable final : public DDLStatement {
public:
    virtual bool is_command() const override { return true; }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, schema_name);
    
    DEF_AST_NODE(DropTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(DropTable);
private:
    DropTable(const AstString *schema_name)
        : schema_name_(DCHECK_NOTNULL(schema_name)) {}
    
    const AstString *schema_name_;
}; // class DropTable

    
////////////////////////////////////////////////////////////////////////////////
/// DML
////////////////////////////////////////////////////////////////////////////////
    
class DMLStatement : public Statement {
public:
    virtual bool is_dml() const { return true; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DMLStatement);
}; // class DMLStatement
    
////////////////////////////////////////////////////////////////////////////////
/// TCL
////////////////////////////////////////////////////////////////////////////////

class TCLStatement final : public Statement {
public:
    enum Txn {
        TXN_BEGIN,
        TXN_ROLLBACK,
        TXN_COMMIT,
    };
    
    DEF_VAL_GETTER(Txn, cmd);
    
    virtual bool is_tcl() const override { return true; }
    
    DEF_AST_NODE(TCLStatement);
    DISALLOW_IMPLICIT_CONSTRUCTORS(TCLStatement);
private:
    TCLStatement(Txn cmd) : cmd_(cmd) {}

    Txn cmd_;
}; // class TCLStatement

////////////////////////////////////////////////////////////////////////////////
/// Utils
////////////////////////////////////////////////////////////////////////////////
    
class UtilStatement : public Statement {
public:
    virtual bool is_util() const override { return true; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(UtilStatement);
protected:
    UtilStatement() {}
}; // class UtilStatement
    
class ShowTables final : public UtilStatement {
public:
    DEF_AST_NODE(ShowTables);
    DISALLOW_IMPLICIT_CONSTRUCTORS(ShowTables);
private:
    ShowTables() {}
}; // class ShowTables
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_AST_NODES_H_
