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
    V(AlterTable) \
    V(AlterTableColumn) \
    V(AlterTableIndex) \
    V(AlterTableName)
    
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
class AlterTableSpec;
    
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

using AstString            = base::ArenaString;
using ColumnDefinitionList = base::ArenaVector<ColumnDefinition *>;
using AlterTableSpecList   = base::ArenaVector<AlterTableSpec *>;
using NameList             = base::ArenaVector<const AstString *>;

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
    
    size_t columns_size() const { return columns_->size(); }
    ColumnDefinition *column(size_t i) { return columns_->at(i); }

    ColumnDefinitionList::const_iterator begin() const { return columns_->begin(); }
    ColumnDefinitionList::const_iterator end() const { return columns_->end(); }

    DEF_PTR_PROP_RW_NOTNULL2(const AstString, schema_name);
    
    DEF_AST_NODE(CreateTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(CreateTable);
private:
    CreateTable(const AstString *schema_name, ColumnDefinitionList *columns)
        : schema_name_(schema_name)
        , columns_(columns) {}

    const AstString *schema_name_;
    ColumnDefinitionList *const columns_;
}; // class CreateTable
    

class AlterTable final : public DDLStatement {
public:
    virtual bool is_command() const override { return true; }
    
    size_t specs_size() const { return specs_->size(); }
    AlterTableSpec *spec(size_t i) { return specs_->at(i); }
    
    AlterTableSpecList::const_iterator begin() const {
        return specs_->begin();
    }
    AlterTableSpecList::const_iterator end() const {
        return specs_->end();
    }
    
    DEF_AST_NODE(AlterTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(AlterTable);
private:
    AlterTable(const AstString *table_name, AlterTableSpecList *specs)
        : table_name_(DCHECK_NOTNULL(table_name))
        , specs_(DCHECK_NOTNULL(specs)) {}
    
    const AstString *table_name_;
    AlterTableSpecList *specs_;
}; // class AlterTable
    
class AlterTableSpec : public AstNode {
public:
    virtual bool alter_column() const { return false; }
    virtual bool alter_index() const { return false; }

    bool is_add() const { return action_ == ADD; }
    bool is_drop() const { return action_ == DROP; }
    bool is_change() const { return action_ == CHANGE; }
    bool is_rename() const { return action_ == DROP; }

    //DEF_PTR_PROP_RW_NOTNULL2(AlterTable, owns);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(AlterTableSpec);
protected:
    enum Action {
        ADD,
        CHANGE,
        DROP,
        RENAME,
    };

    AlterTableSpec(Action action)
        : action_(action) {}
    
    const Action action_;
    //AlterTable *owns_ = nullptr;
}; // class AlterTable


class AlterTableColumn final : public AlterTableSpec {
public:
    virtual bool alter_column() const override { return true; }
    
    bool add_after() const { return is_add() && after_; }
    bool add_first() const { return is_add() && !after_; }
    
    const AstString *from_col_name() const {
        return is_rename() ? old_name_ : AstString::kEmpty;
    }

    const AstString *to_col_name() const {
        return is_rename() ? new_name_ : AstString::kEmpty;
    }
    
    const AstString *pos_col_name() const {
        return is_add() || is_change() ? new_name_ : AstString::kEmpty;
    }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, old_name);
    DEF_PTR_GETTER_NOTNULL(const AstString, new_name);
    
    size_t columns_size() const { return columns_->size(); }
    ColumnDefinition *column(size_t i) { return columns_->at(i); }
    
    ColumnDefinitionList::const_iterator begin() const {
        return columns_->begin();
    }
    ColumnDefinitionList::const_iterator end() const {
        return columns_->end();
    }
    
    DEF_AST_NODE(AlterTableColumn);
    DISALLOW_IMPLICIT_CONSTRUCTORS(AlterTableColumn);
private:
    AlterTableColumn(ColumnDefinitionList *columns,
                     bool after, const AstString *col_name)
        : AlterTableSpec(ADD)
        , columns_(DCHECK_NOTNULL(columns))
        , after_(after)
        , new_name_(col_name) {}
    
    AlterTableColumn(const AstString *old_col_name,
                     ColumnDefinitionList *columns,
                     bool after, const AstString *col_name)
        : AlterTableSpec(CHANGE)
        , columns_(DCHECK_NOTNULL(columns))
        , old_name_(old_col_name)
        , new_name_(col_name)
        , after_(after) {}
    
    AlterTableColumn(const AstString *from_col_name,
                     const AstString *to_col_name)
        : AlterTableSpec(RENAME)
        , old_name_(from_col_name)
        , new_name_(to_col_name) {}
    
    AlterTableColumn(const AstString *drop_col_name)
        : AlterTableSpec(DROP)
        , old_name_(drop_col_name) {}
    
    ColumnDefinitionList *columns_ = nullptr;
    const AstString *old_name_ = AstString::kEmpty;
    const AstString *new_name_ = AstString::kEmpty;
    bool after_ = false;
}; // class AlterTableColumn
    

class AlterTableIndex final : public AlterTableSpec {
public:
    virtual bool alter_index() const override { return true; }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, old_name);
    DEF_PTR_GETTER_NOTNULL(const AstString, new_name);
    DEF_VAL_GETTER(SQLKeyType, key);
    
    bool drop_primary_key() const {
        return is_drop() && key_ == SQL_PRIMARY_KEY;
    }
    
    const AstString *drop_idx_name() const {
        return is_drop() ? old_name() : AstString::kEmpty;
    }
    
    const AstString *from_idx_name() const {
        return is_rename() ? old_name() : AstString::kEmpty;
    }
    
    const AstString *to_idx_name() const {
        return is_rename() ? new_name() : AstString::kEmpty;
    }
    
    size_t col_names_size() const { return col_names_->size(); }
    const AstString *col_name(size_t i) { return col_names_->at(i); }
    
    DEF_AST_NODE(AlterTableIndex);
    DISALLOW_IMPLICIT_CONSTRUCTORS(AlterTableIndex);
private:
    AlterTableIndex(const AstString *index_name, SQLKeyType key_type,
                    NameList *col_names)
        : AlterTableSpec(ADD)
        , col_names_(DCHECK_NOTNULL(col_names))
        , new_name_(index_name)
        , key_(key_type) {}
    
    AlterTableIndex(const AstString *from_idx_name,
                    const AstString *to_idx_name)
        : AlterTableSpec(RENAME)
        , old_name_(from_idx_name)
        , new_name_(to_idx_name) {}
    
    AlterTableIndex(const AstString *drop_idx_name, bool primary_key)
        : AlterTableSpec(DROP)
        , old_name_(drop_idx_name)
        , key_(primary_key ? SQL_PRIMARY_KEY : SQL_NOT_KEY) {
    }
    
    NameList *col_names_ = nullptr;
    const AstString *old_name_ = AstString::kEmpty;
    const AstString *new_name_ = AstString::kEmpty;
    SQLKeyType key_ = SQL_NOT_KEY;
}; // class AlterTableIndex


class AlterTableName final : public AlterTableSpec {
public:
    DEF_PTR_GETTER_NOTNULL(const AstString, new_name);
    
    DEF_AST_NODE(AlterTableName);
    DISALLOW_IMPLICIT_CONSTRUCTORS(AlterTableName);
private:
    AlterTableName(const AstString *new_table_name)
        : AlterTableSpec(RENAME)
        , new_name_(new_table_name) {}
    
    const AstString *new_name_ = AstString::kEmpty;
}; // class AlterTableName


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
