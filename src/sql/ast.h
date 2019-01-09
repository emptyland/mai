#ifndef MAI_SQL_AST_NODES_H_
#define MAI_SQL_AST_NODES_H_

#include "sql/types.h"
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
    V(DropTable) \
    V(ColumnDefinition) \
    V(TypeDefinition) \
    V(AlterTableColumn)
    
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
    using Columns = base::ArenaVector<ColumnDefinition *>;
    
    virtual bool is_command() const override { return true; }
    
    void AddColumn(ColumnDefinition *column) {
        columns_.push_back(column);
    }
    
    size_t columns_size() const { return columns_.size(); }
    ColumnDefinition *column(size_t i) { return columns_[i]; }

    Columns::const_iterator begin() const { return columns_.begin(); }
    Columns::const_iterator end() const { return columns_.end(); }

    DEF_PTR_PROP_RW_NOTNULL2(const AstString, schema_name);
    
    DEF_AST_NODE(CreateTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(CreateTable);
private:
    CreateTable(const AstString *schema_name, base::Arena *arena)
        : schema_name_(schema_name)
        , columns_(arena) {}

    const AstString *schema_name_;
    Columns columns_;
}; // class CreateTable
    
    
class AlterTable : public DDLStatement {
public:
    virtual bool is_command() const override { return true; }
    virtual bool alter_column() const { return false; }
    virtual bool alter_index() const { return false; }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, table_name);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(AlterTable);
protected:
    AlterTable(const AstString *table_name)
        : table_name_(table_name) {}
    
    const AstString *table_name_;
}; // class AlterTable


class AlterTableColumn : public AlterTable {
public:
    using Columns = base::ArenaVector<ColumnDefinition *>;
    
    virtual bool alter_column() const override { return true; }
    bool add() const { return add_; }
    bool drop() const { return !add_; }
    
    bool add_after() const { return add() && after_; }
    bool add_first() const { return add() && !after_; }
    
    void AddColumn(ColumnDefinition *column) {
        columns_.push_back(column);
    }
    
    size_t columns_size() const { return columns_.size(); }
    ColumnDefinition *column(size_t i) { return columns_[i]; }
    
    Columns::const_iterator begin() const { return columns_.begin(); }
    Columns::const_iterator end() const { return columns_.end(); }
    
private:
    Columns columns_;
    const AstString *column_name_;
    bool add_;
    bool after_;
}; // class AlterTableColumn
    

    
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
    
    
class ColumnDefinition final : public AstNode {
public:
    DEF_PTR_GETTER_NOTNULL(const AstString, name);
    DEF_PTR_GETTER_NOTNULL(TypeDefinition, type);
    DEF_VAL_GETTER(bool, is_not_null);
    DEF_VAL_GETTER(bool, auto_increment);
    DEF_VAL_GETTER(SQLKeyType, key);
    DEF_PTR_PROP_RW_NOTNULL2(const AstString, comment);
    
    bool is_key() const { return key_ == SQL_KEY; }
    bool is_unique_key() const { return key_ == SQL_UNIQUE_KEY; }
    bool is_primary_key() const { return key_ == SQL_PRIMARY_KEY; }
    bool is_not_key() const { return key_ == SQL_NOT_KEY; }
    
    DEF_AST_NODE(ColumnDefinition);
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnDefinition);
private:
    ColumnDefinition(const AstString *name,
                      TypeDefinition *type,
                      bool is_not_null,
                      bool auto_increment,
                      SQLKeyType key)
        : name_(name)
        , type_(type)
        , is_not_null_(is_not_null)
        , key_(key) {}
    
    const AstString *name_;
    TypeDefinition *type_;
    bool is_not_null_ = false;
    bool auto_increment_ = false;
    SQLKeyType key_ = SQL_NOT_KEY;
    const AstString *comment_ = AstString::kEmpty;
}; // class ColumnDeclaration

    
class TypeDefinition final : public AstNode {
public:
    DEF_VAL_GETTER(SQLType, code);
    DEF_VAL_GETTER(int, fixed_size);
    DEF_VAL_GETTER(int, float_size);
    
    DEF_AST_NODE(TypeDefinition);
    DISALLOW_IMPLICIT_CONSTRUCTORS(TypeDefinition);
    
private:
    TypeDefinition(SQLType code, int fixed_size, int float_size)
        : code_(code)
        , fixed_size_(fixed_size)
        , float_size_(float_size) {}

    SQLType code_;
    int fixed_size_;
    int float_size_;
}; // class TypeDeclaration

    
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
