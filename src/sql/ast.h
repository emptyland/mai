#ifndef MAI_SQL_AST_NODES_H_
#define MAI_SQL_AST_NODES_H_

#include "sql/types.h"
#include "base/arena-utils.h"
#include "base/base.h"

struct YYLTYPE;

namespace mai {
    
namespace sql {
    
#define DEFINE_AST_NODES(V) \
    DEFINE_DDL_NODES(V) \
    DEFINE_DML_NODES(V) \
    DEFINE_TCL_NODES(V) \
    DEFINE_EXPR_NODES(V) \
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
    V(Select) \
    V(Insert) \
    V(Update) \
    V(NameRelation) \
    V(JoinRelation)
    
#define DEFINE_EXPR_NODES(V) \
    V(ProjectionColumn) \
    V(Identifier) \
    V(Literal) \
    V(Subquery) \
    V(Placeholder) \
    V(UnaryExpression) \
    V(BinaryExpression) \
    V(Assignment) \
    V(Comparison) \
    V(MultiExpression) \
    V(Call) \
    V(Aggregate)
    
#define DEFINE_UTIL_NODES(V) \
    V(ShowTables)
    
#define DEFINE_TCL_NODES(V) V(TCLStatement)
    
using AstString = base::ArenaString;
    
namespace ast {
    
#define PRE_DECLARE_NODE(name) class name;
DEFINE_AST_NODES(PRE_DECLARE_NODE)
#undef PRE_DECLARE_NODE

class Factory;
class Visitor;
class Block;
class Statement;
class AlterTableSpec;
class Expression;
    
struct Location {
    int begin_line   = 0;
    int begin_column = 0;
    int end_line     = 0;
    int end_column   = 0;
    
    Location(const YYLTYPE &yyl);
    
    static Location Concat(const YYLTYPE &first, const YYLTYPE &last);
};
    
class AstNode {
public:
    enum Kind {
    #define DECL_ENUM(name) k##name,
        DEFINE_AST_NODES(DECL_ENUM)
    #undef  DECL_ENUM
    }; // enum Kind
    
    virtual void Accept(Visitor *v) = 0;
    virtual Kind kind() const = 0;
    virtual bool is_ddl() const { return false; }
    virtual bool is_dml() const { return false; }
    virtual bool is_tcl() const { return false; }
    virtual bool is_util() const { return false; }
    virtual bool is_statement() const { return false; }
    virtual bool is_command() const { return false; }
    virtual bool is_expression() const { return false; }
    
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
    virtual void Accept(Visitor *v) override { v->Visit##clazz(this); } \
    virtual Kind kind() const override { return k##clazz; } \
    static const clazz *Cast(const AstNode *n) { \
        return n->kind() == k##clazz ? down_cast<const clazz>(n) : nullptr; \
    } \
    static clazz *Cast(AstNode *n) { \
        return n->kind() == k##clazz ? down_cast<clazz>(n) : nullptr; \
    } \
    friend class Factory;

using ColumnDefinitionList = base::ArenaVector<ColumnDefinition *>;
using AlterTableSpecList   = base::ArenaVector<AlterTableSpec *>;
using NameList             = base::ArenaVector<const AstString *>;
using ExpressionList       = base::ArenaVector<Expression *>;
using ProjectionColumnList = base::ArenaVector<ProjectionColumn *>;
using AssignmentList       = base::ArenaVector<Assignment *>;
using RowValuesList        = base::ArenaVector<ExpressionList *>;
    
////////////////////////////////////////////////////////////////////////////////
/// class Visitor
////////////////////////////////////////////////////////////////////////////////

class Visitor {
public:
    Visitor() {}
    virtual ~Visitor() {}
    
#define DECL_METHOD(name) virtual void Visit##name(name *node) {}
    DEFINE_AST_NODES(DECL_METHOD)
#undef DECL_METHOD
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Visitor);
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
    
    friend class Factory;
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

    DEF_PTR_PROP_RW_NOTNULL2(const Identifier, table_name);
    DEF_PTR_PROP_RW_NOTNULL2(const AstString, engine_name);
    
    DEF_AST_NODE(CreateTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(CreateTable);
private:
    CreateTable(const Identifier *table_name, ColumnDefinitionList *columns)
        : table_name_(table_name)
        , columns_(columns) {}

    const Identifier *table_name_;
    ColumnDefinitionList *const columns_;
    const AstString *engine_name_ = AstString::kEmpty;
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
    
    DEF_PTR_GETTER_NOTNULL(const Identifier, table_name);
    
    DEF_AST_NODE(AlterTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(AlterTable);
private:
    AlterTable(const Identifier *table_name, AlterTableSpecList *specs)
        : table_name_(DCHECK_NOTNULL(table_name))
        , specs_(DCHECK_NOTNULL(specs)) {}
    
    const Identifier *table_name_;
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
    
    DEF_PTR_GETTER_NOTNULL(const Identifier, table_name);
    
    DEF_AST_NODE(DropTable);
    DISALLOW_IMPLICIT_CONSTRUCTORS(DropTable);
private:
    DropTable(const Identifier *table_name)
        : table_name_(DCHECK_NOTNULL(table_name)) {}
    
    const Identifier *table_name_;
}; // class DropTable
    
    
class ColumnDefinition final : public AstNode {
public:
    DEF_PTR_GETTER_NOTNULL(const AstString, name);
    DEF_PTR_GETTER_NOTNULL(TypeDefinition, type);
    DEF_VAL_PROP_RW(bool, is_not_null);
    DEF_VAL_PROP_RW(bool, auto_increment);
    DEF_VAL_PROP_RW(SQLKeyType, key);
    DEF_PTR_PROP_RW(Expression, default_value);
    DEF_PTR_PROP_RW_NOTNULL2(const AstString, comment);
    
    bool is_key() const { return key_ == SQL_KEY; }
    bool is_unique_key() const { return key_ == SQL_UNIQUE_KEY; }
    bool is_primary_key() const { return key_ == SQL_PRIMARY_KEY; }
    bool is_not_key() const { return key_ == SQL_NOT_KEY; }
    
    DEF_AST_NODE(ColumnDefinition);
    DISALLOW_IMPLICIT_CONSTRUCTORS(ColumnDefinition);
private:
    ColumnDefinition(const AstString *name,
                      TypeDefinition *type)
        : name_(name)
        , type_(type) {}
    
    const AstString *name_;
    TypeDefinition *type_;
    bool is_not_null_ = false;
    bool auto_increment_ = false;
    SQLKeyType key_ = SQL_NOT_KEY;
    Expression *default_value_;
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
    virtual bool is_dml() const override { return true; }
    virtual bool is_query() const { return false; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(DMLStatement);
protected:
    DMLStatement() {}
}; // class DMLStatement
    

class Query : public DMLStatement {
public:
    virtual bool is_query() const override { return true; }
    virtual bool is_select() const { return false; }
    virtual bool is_join() const { return false; }
    virtual bool is_name() const { return false; }

    DEF_PTR_PROP_RW_NOTNULL2(const AstString, alias);

    DISALLOW_IMPLICIT_CONSTRUCTORS(Query);
protected:
    Query(const AstString *alias) : alias_(DCHECK_NOTNULL(alias)) {}
    
    const AstString *alias_;
}; // class Query
    
    
class Select final : public Query {
public:
    virtual bool is_select() const override { return true; }
    
    DEF_PTR_GETTER_NOTNULL(ProjectionColumnList, columns);
    DEF_PTR_PROP_RW(Query, from_clause);
    DEF_PTR_PROP_RW(Expression, where_clause);
    DEF_PTR_PROP_RW(Expression, having_clause);
    DEF_VAL_PROP_RW(bool, order_by_desc);
    DEF_PTR_PROP_RW(ExpressionList, order_by_clause);
    DEF_PTR_PROP_RW(ExpressionList, group_by_clause);
    DEF_VAL_PROP_RW(bool, for_update);
    DEF_VAL_PROP_RW(int, limit_val);
    DEF_VAL_PROP_RW(int, offset_val);
    
    DEF_AST_NODE(Select);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Select);
private:
    Select(bool distinct, ProjectionColumnList *columns, const AstString *alias)
        : Query(alias)
        , distinct_(distinct)
        , columns_(DCHECK_NOTNULL(columns)) {}

    bool distinct_;
    ProjectionColumnList *columns_;
    Query *from_clause_ = nullptr;
    Expression *where_clause_ = nullptr;
    Expression *having_clause_ = nullptr;
    bool order_by_desc_ = false;
    ExpressionList *order_by_clause_ = nullptr;
    ExpressionList *group_by_clause_ = nullptr;
    int limit_val_ = 0;
    int offset_val_ = 0;
    bool for_update_ = false;
}; // class Select


class NameRelation final : public Query {
public:
    virtual bool is_statement() const override { return false; }
    virtual bool is_command() const override { return false; }
    virtual bool is_name() const override { return true; }
    
    inline const AstString *prefix() const;
    inline const AstString *name() const;
    
    DEF_AST_NODE(NameRelation);
    DISALLOW_IMPLICIT_CONSTRUCTORS(NameRelation);
private:
    NameRelation(Identifier *ref_name, const AstString *alias)
        : Query(alias)
        , ref_name_(DCHECK_NOTNULL(ref_name)) {}
    
    Identifier *ref_name_;
}; // class NameRelation
    
    
class JoinRelation final : public Query {
public:
    virtual bool is_statement() const override { return false; }
    virtual bool is_command() const override { return false; }
    virtual bool is_join() const override { return true; }
    
    DEF_VAL_GETTER(SQLJoinKind, join_kind);
    DEF_PTR_GETTER_NOTNULL(Query, lhs);
    DEF_PTR_GETTER_NOTNULL(Query, rhs);
    
    DEF_AST_NODE(JoinRelation);
    DISALLOW_IMPLICIT_CONSTRUCTORS(JoinRelation);
private:
    JoinRelation(SQLJoinKind join_kind, Query *lhs, Query *rhs, Expression *on,
                 const AstString *alias)
        : Query(alias)
        , join_kind_(join_kind)
        , lhs_(DCHECK_NOTNULL(lhs))
        , rhs_(DCHECK_NOTNULL(rhs))
        , on_(on) {}
    
    SQLJoinKind join_kind_;
    Query *lhs_;
    Query *rhs_;
    Expression *on_;
}; // class JoinRelation
    
    
class Insert final : public DMLStatement {
public:
    
    DEF_VAL_GETTER(bool, overwrite);
    DEF_PTR_GETTER_NOTNULL(const Identifier, table_name);
    DEF_PTR_PROP_RW(NameList, col_names);
    DEF_PTR_PROP_RW(RowValuesList, row_values_list);
    DEF_PTR_PROP_RW(AssignmentList, on_duplicate_clause);
    DEF_PTR_PROP_RW(Query, select_clause);
    
    inline void SetAssignmentList(AssignmentList *a, base::Arena *arena);

    DEF_AST_NODE(Insert);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Insert);
private:
    Insert(const Identifier *table_name, bool overwrite)
        : overwrite_(overwrite)
        , table_name_(table_name) {}
    
    bool overwrite_;
    const Identifier *table_name_;
    NameList *col_names_ = nullptr;
    RowValuesList *row_values_list_ = nullptr;
    AssignmentList *on_duplicate_clause_ = nullptr;
    Query *select_clause_ = nullptr;
}; // class Insert
    
    
class Update final : public DMLStatement {
public:

    DEF_PTR_GETTER_NOTNULL(const Identifier, table_name);
    DEF_PTR_GETTER_NOTNULL(AssignmentList, assignments);
    DEF_VAL_PROP_RW(bool, order_by_desc);
    DEF_PTR_PROP_RW(ExpressionList, order_by_clause);
    DEF_PTR_PROP_RW(Expression, where_clause);
    DEF_VAL_PROP_RW(int, limit_val);

    DEF_AST_NODE(Update);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Update);
private:
    Update(const Identifier *table_name, AssignmentList *assignments)
        : table_name_(DCHECK_NOTNULL(table_name))
        , assignments_(DCHECK_NOTNULL(assignments)) {}
    
    const Identifier *table_name_;
    AssignmentList *assignments_;
    Expression *where_clause_ = nullptr;
    bool order_by_desc_ = false;
    ExpressionList *order_by_clause_ = nullptr;
    int limit_val_ = 0;
}; // class Update
    
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
/// Expressions
////////////////////////////////////////////////////////////////////////////////
    
class Expression : public AstNode {
public:
    virtual bool is_expression() const override { return true; }
    
    DEF_VAL_GETTER(Location, location);
    
    bool is_unary() const { return operands_count() == 1; }
    bool is_binary() const { return operands_count() == 2; }
    
    virtual int operands_count() const { return 0; }
    
    virtual bool is_literal() const { return false; }
    virtual bool is_subquery() const { return false; }
    virtual bool is_comparison() const { return false; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(Expression);
protected:
    Expression(const Location &location)
        : location_(location) {}
    
    Location location_;
}; // class Expression
    
    
class ProjectionColumn final : public Expression {
public:
    virtual int operands_count() const override {
        return expr_->operands_count();
    }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, alias);
    DEF_PTR_GETTER_NOTNULL(Expression, expr);
    
    DEF_AST_NODE(ProjectionColumn);
    DISALLOW_IMPLICIT_CONSTRUCTORS(ProjectionColumn);
private:
    ProjectionColumn(const AstString *alias, Expression *expr,
                     const Location &location)
        : Expression(location)
        , alias_(DCHECK_NOTNULL(alias))
        , expr_(DCHECK_NOTNULL(expr)) {}

    const AstString *alias_;
    Expression *expr_;
}; // class ProjectionColumn
    
    
class Placeholder final : public Expression {
public:
    enum Type {
        STAR,
        PARAM,
    };
    
    bool is_star() const { return type_ == STAR; }
    bool is_param() const { return type_ == PARAM; }
    
    DEF_AST_NODE(Placeholder);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Placeholder);
private:
    Placeholder(Type type, const Location &location)
        : Expression(location)
        , type_(type) {}
    
    Type type_;
}; // class Placeholder


class Identifier final : public Expression {
public:
    virtual int operands_count() const override { return 1; }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, prefix_name);
    DEF_PTR_GETTER_NOTNULL(const AstString, name);
    DEF_PTR_GETTER(Placeholder, placeholder);
    
    std::string ToString() const {
        if (placeholder_) {
            return "?";
        }
        if (prefix_name_->empty()) {
            return name_->ToString();
        }
        return prefix_name_->ToString() + "." + name_->ToString();
    }

    DEF_AST_NODE(Identifier);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Identifier);
private:
    Identifier(const AstString *prefix, const AstString *name,
               const Location &location)
        : Expression(location)
        , prefix_name_(prefix)
        , name_(name) {}
    
    Identifier(const AstString *prefix, Placeholder *placeholder,
               const Location &location)
        : Expression(location)
        , prefix_name_(prefix)
        , placeholder_(placeholder) {}
    
    const AstString *prefix_name_;
    const AstString *name_ = AstString::kEmpty;
    Placeholder *placeholder_ = nullptr;
}; // class Identifier
    
    
class Literal final : public Expression {
public:
    enum Type {
        STRING,
        INTEGER,
        APPROX,
        NULL_VAL,
        DEFAULT_PLACEHOLDER,
    };
    virtual int operands_count() const override { return 1; }
    virtual bool is_literal() const override { return true; }
    
    bool is_string_val() const { return type_ == STRING; }
    bool is_integer_val() const { return type_ == INTEGER; }
    bool is_approx_val() const { return type_ == APPROX; }
    bool is_null_val() const { return type_ == NULL_VAL; }
    bool is_default_placeholder() const { return type_ == DEFAULT_PLACEHOLDER; }
    
    DEF_VAL_GETTER(Type, type);
    
    int64_t integer_val() const {
        DCHECK(is_integer_val()); return int_val_;
    }
    
    double approx_val() const {
        DCHECK(is_approx_val()); return approx_val_;
    }
    
    const AstString *string_val() const {
        DCHECK(is_string_val()); return str_val_;
    }

    DEF_AST_NODE(Literal);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Literal);
private:
    Literal(int64_t val, const Location &location)
        : Expression(location)
        , type_(INTEGER)
        , int_val_(val) {}
    
    Literal(double val, const Location &location)
        : Expression(location)
        , type_(APPROX)
        , approx_val_(val) {}
    
    Literal(const AstString *val, const Location &location)
        : Expression(location)
        , type_(STRING)
        , str_val_(val) {}
    
    Literal(Type type, const Location &location)
        : Expression(location)
        , type_(type) {
        DCHECK_EQ(type, NULL_VAL);
        DCHECK_EQ(type, DEFAULT_PLACEHOLDER);
    }
    
    Type type_;
    union {
        int64_t int_val_;
        double  approx_val_;
        const AstString *str_val_;
    };
}; // class Literal
    
    
class Call : public Expression {
public:
    bool is_udf() const { return name_->full(); }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, name);
    DEF_VAL_GETTER(SQLFunction, fnid);
    DEF_PTR_GETTER(ExpressionList, parameters);
    
    size_t parameters_size() const {
        return !parameters_ ? 0 : parameters_->size();
    }
    
    Expression *parameter(size_t i) const {
        DCHECK_LT(i, parameters_size());
        return parameters_->at(i);
    }
    
    DEF_AST_NODE(Call);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Call);
protected:
    Call(const AstString *name, ExpressionList *params, const Location &location)
        : Expression(location)
        , name_(DCHECK_NOTNULL(name))
        , parameters_(params) {
        DCHECK(name_->full());
    }
    
    Call(SQLFunction fnid, ExpressionList *params, const Location &location)
        : Expression(location)
        , fnid_(fnid)
        , parameters_(params) {
    }
    
private:
    const AstString *name_ = AstString::kEmpty;
    SQLFunction fnid_;
    ExpressionList *parameters_;
}; // class Call
    

class Aggregate : public Call {
public:

    DEF_AST_NODE(Aggregate);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Aggregate);
private:
    Aggregate(const AstString *name, bool distinct, ExpressionList *params,
              const Location &location)
        : Call(name, params, location)
        , distinct_(distinct) {}
    
    Aggregate(SQLFunction fnid, bool distinct, ExpressionList *params,
              const Location &location)
        : Call(fnid, params, location)
        , distinct_(distinct) {
        DCHECK_GE(fnid, 0);
        DCHECK_LT(fnid, SQL_MAX_F);
        DCHECK(kSQLFunctionDesc[fnid].aggregate);
    }
    
    bool distinct_;
}; // class Aggregate


class Subquery final : public Expression {
public:
    DEF_PTR_GETTER_NOTNULL(Query, query);
    
    bool match_any() const { return !match_all_; }
    bool match_all() const { return  match_all_; }
    
    DEF_AST_NODE(Subquery);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Subquery);
private:
    Subquery(bool match_all, Query *query, const Location &location)
        : Expression(location)
        , match_all_(match_all)
        , query_(DCHECK_NOTNULL(query)) {}
    
    bool match_all_;
    Query *query_;
}; // class Subquery
    
    
template<int N>
class FixedOperand : public Expression {
public:
    virtual int operands_count() const override { return N; }
    
    SQLOperator op() const { return op_; }
    
    Expression *operand(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, N);
        return operands_[i];
    }

    Expression *rhs() const {
        DCHECK_EQ(2, N);
        return operands_[1];
    }
    
    Expression *lhs() const {
        DCHECK_EQ(2, N);
        return operands_[0];
    }
    
protected:
    FixedOperand(SQLOperator op, const Location &location)
        : Expression(location)
        , op_(op) {
        for (int i = 0; i < N; ++i) {
            operands_[i] = nullptr;
        }
    }
    
    void set_lhs(Expression *expr) { set_operand(0, expr); }
    void set_rhs(Expression *expr) { set_operand(1, expr); }
    
    void set_operand(int i, Expression *expr) {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, N);
        operands_[i] = DCHECK_NOTNULL(expr);
    }

    SQLOperator op_;
    Expression *operands_[N];
}; // template<int N> class OperandExpr
    
    
class MultiOperand : public Expression {
public:
    virtual int operands_count() const override {
        return rhs_ ? 1 + static_cast<int>(rhs_->size()) : 1;
    }
    
    DEF_PTR_GETTER_NOTNULL(Expression, lhs);
    DEF_PTR_GETTER(ExpressionList, rhs);
    
    Expression *operand(int i) const {
        DCHECK_GE(i, 0);
        DCHECK_LT(i, operands_count());
        return i == 0 ? lhs_ : rhs_->at(1 + i);
    }
    
    SQLOperator op() const { return op_; }
protected:
    MultiOperand(SQLOperator op, Expression *lhs, ExpressionList *rhs,
                 const Location &location)
        : Expression(location)
        , op_(op)
        , lhs_(DCHECK_NOTNULL(lhs))
        , rhs_(rhs) {}
    
    void set_lhs(Expression *expr) { lhs_ = DCHECK_NOTNULL(expr); }
    void set_rhs(ExpressionList *expr_list) { rhs_ = expr_list; }
    
    SQLOperator op_;
    Expression *lhs_;
    ExpressionList *rhs_;
}; // class MultiOperand
    

class UnaryExpression final : public FixedOperand<1> {
public:
    static bool IsUnaryOperator(SQLOperator op);

    DEF_AST_NODE(UnaryExpression);
    DISALLOW_IMPLICIT_CONSTRUCTORS(UnaryExpression);
private:
    UnaryExpression(SQLOperator op, Expression *operand,
                    const Location &location)
        : FixedOperand(op, location) {
        DCHECK(IsUnaryOperator(op));
        set_operand(0, operand);
    }
}; // class UnaryExpression
    
    
class BinaryExpression final : public FixedOperand<2> {
public:
    static bool IsBinaryOperator(SQLOperator op);
    
    DEF_AST_NODE(BinaryExpression);
    DISALLOW_IMPLICIT_CONSTRUCTORS(BinaryExpression);
private:
    BinaryExpression(SQLOperator op, Expression *lhs, Expression *rhs,
                     const Location &location)
        : FixedOperand(op, location) {
        DCHECK(IsBinaryOperator(op));
        set_lhs(lhs);
        set_rhs(rhs);
    }
}; // class BinaryExpression
    
    
class Assignment final : public FixedOperand<1> {
public:
    virtual int operands_count() const override { return 2; }
    
    DEF_PTR_GETTER_NOTNULL(const AstString, name);
    Expression *rval() const { return operand(0); }

    DEF_AST_NODE(Assignment);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Assignment);
private:
    Assignment(const AstString *name, Expression *rval,
               const Location &location)
        : FixedOperand(SQL_ASSIGN, location)
        , name_(name) { set_operand(0, rval); }

    const AstString *name_;
}; // class Assignment
    
    
class Comparison final : public FixedOperand<2> {
public:
    virtual bool is_comparison() const override { return true; }
    
    static bool IsComparisonOperator(SQLOperator op);
    
    DEF_AST_NODE(Comparison);
    DISALLOW_IMPLICIT_CONSTRUCTORS(Comparison);
private:
    Comparison(SQLOperator op, Expression *lhs, Expression *rhs,
               const Location &location)
        : FixedOperand(op, location) {
        DCHECK(IsComparisonOperator(op));
        set_lhs(lhs);
        set_rhs(rhs);
    }
}; // class Comparison
    
    
class MultiExpression final : public MultiOperand {
public:
    DEF_AST_NODE(MultiExpression);
    DISALLOW_IMPLICIT_CONSTRUCTORS(MultiExpression);
private:
    MultiExpression(SQLOperator op, Expression *lhs, ExpressionList *rhs,
                    const Location &location)
        : MultiOperand(op, lhs, rhs, location) {}
}; // class MultiExpression


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
    
    
////////////////////////////////////////////////////////////////////////////////
/// Inlines
////////////////////////////////////////////////////////////////////////////////
    
inline const AstString *NameRelation::prefix() const { return ref_name_->prefix_name(); }
inline const AstString *NameRelation::name() const { return ref_name_->name(); }
    
    
inline void Insert::SetAssignmentList(AssignmentList *a, base::Arena *arena) {
    col_names_ = new (arena) NameList(a->size(), arena);
    row_values_list_ = new (arena) RowValuesList(1, arena);
    auto values = new (arena) ExpressionList(a->size(), arena);
    for (int i = 0; i < a->size(); ++i) {
        (*values)[i]     = a->at(i)->rval();
        (*col_names_)[i] = a->at(i)->name();
    }
    
    (*row_values_list_)[0] = values;
}
    
} // namespace ast
    
} // namespace sql
    
} // namespace mai

#endif // MAI_SQL_AST_NODES_H_
