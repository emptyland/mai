#ifndef MAI_SQL_AST_FACTORY_H_
#define MAI_SQL_AST_FACTORY_H_

#include "sql/ast.h"
#include "base/slice.h"
#include <stdarg.h>

namespace mai {
    
namespace sql {
    
namespace ast {
    
class Factory final {
public:
    Factory(base::Arena *arena) : arena_(arena) {}
    ~Factory() {}
    
    Block *NewBlock() { return new (arena_) Block(arena_); }

    ////////////////////////////////////////////////////////////////////////////
    // DDL
    ////////////////////////////////////////////////////////////////////////////
    CreateTable *NewCreateTable(const Identifier *table_name,
                                ColumnDefinitionList *cols) {
        return new (arena_) CreateTable(table_name, cols);
    }
    
    AlterTable *NewAlterTable(const Identifier *table_name,
                              AlterTableSpecList *spces) {
        return new (arena_) AlterTable(table_name, spces);
    }

    AlterTableColumn *NewAlterTableAddColumn(ColumnDefinitionList *cols) {
        return new (arena_) AlterTableColumn(cols, false, AstString::kEmpty);
    }
    
    AlterTableColumn *NewAlterTableAddColumn(ColumnDefinition *col,
                                             bool after,
                                             const AstString *col_name) {
        auto cols = NewColumnDefinitionList(col);
        return new (arena_) AlterTableColumn(cols, after, col_name);
    }
    
    AlterTableColumn *NewAlterTableChangeColumn(const AstString *old_col_name,
                                                ColumnDefinition *col,
                                                bool after,
                                                const AstString *col_name) {
        auto cols = NewColumnDefinitionList(col);
        return new (arena_) AlterTableColumn(old_col_name, cols, after,
                                             col_name);
    }
    
    AlterTableColumn *NewAlterTableRenameColumn(const AstString *from_col_name,
                                                const AstString *to_col_name) {
        return new (arena_) AlterTableColumn(from_col_name, to_col_name);
    }
    
    AlterTableColumn *NewAlterTableDropColumn(const AstString *drop_col_name) {
        return new (arena_) AlterTableColumn(drop_col_name);
    }
    
    AlterTableIndex *NewAlterTableAddIndex(const AstString *index_name,
                                           SQLKeyType key,
                                           NameList *col_names) {
        return new (arena_) AlterTableIndex(index_name, key, col_names);
    }
    
    AlterTableIndex *NewAlterTableDropIndex(const AstString *drop_idx_name,
                                            bool primary_key) {
        return new (arena_) AlterTableIndex(drop_idx_name, primary_key);
    }
    
    AlterTableIndex *NewAlterTableRenameIndex(const AstString *from_idx_name,
                                              const AstString *to_idx_name) {
        return new (arena_) AlterTableIndex(from_idx_name, to_idx_name);
    }
    
    AlterTableName *NewAlterTableRename(const AstString *new_table_name) {
        return new (arena_) AlterTableName(new_table_name);
    }

    DropTable *NewDropTable(const Identifier *table_name) {
        return new (arena_) DropTable(table_name);
    }
    
    ColumnDefinition *NewColumnDefinition(const AstString *name,
                                            TypeDefinition *type) {
        return new (arena_) ColumnDefinition(name, type);
    }
    
    TypeDefinition *NewTypeDefinition(SQLType type, int fixed_size = 0,
                                      int float_size = 0) {
        return new (arena_) TypeDefinition(type, fixed_size, float_size);
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // TCL
    ////////////////////////////////////////////////////////////////////////////
    TCLStatement *NewTCLStatement(TCLStatement::Txn cmd) {
        return new (arena_) TCLStatement(cmd);
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // DML
    ////////////////////////////////////////////////////////////////////////////
    Select *NewSelect(bool distinct, ProjectionColumnList *cols,
                      const AstString *alias) {
        return new (arena_) Select(distinct, cols, alias);
    }
    
    NameRelation *NewNameRelation(Identifier *name, const AstString *alias) {
        return new (arena_) NameRelation(name, alias);
    }
    
    JoinRelation *NewJoinRelation(Query *lhs, SQLJoinKind join, Query *rhs,
                                  Expression *on_expr,
                                  const AstString *alias) {
        return new (arena_) JoinRelation(join, lhs, rhs, on_expr, alias);
    }
    
    Insert *NewInsert(bool overwrite, const Identifier *table_name) {
        return new (arena_) Insert(table_name, overwrite);
    }
    
    Update *NewUpdate(const Identifier *table_name,
                      AssignmentList *assignments) {
        return new (arena_) Update(table_name, assignments);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Expressions
    ////////////////////////////////////////////////////////////////////////////
    ProjectionColumn *NewProjectionColumn(Expression *expr,
                                          const AstString *alias,
                                          const Location &location) {
        return new (arena_) ProjectionColumn(alias, expr, location);
    }
    
    Identifier *NewIdentifier(const AstString *prefix, const AstString *name,
                              const Location &location) {
        return new (arena_) Identifier(prefix, name, location);
    }
    
    Identifier *NewIdentifierWithPlaceholder(const AstString *prefix,
                                             Placeholder *placeholder,
                                             const Location &location) {
        return new (arena_) Identifier(prefix, placeholder, location);
    }
    
    Placeholder *NewStarPlaceholder(const Location &location) {
        return new (arena_) Placeholder(Placeholder::STAR, location);
    }
    
    Placeholder *NewParamPlaceholder(const Location &location) {
        return new (arena_) Placeholder(Placeholder::PARAM, location);
    }
    
    Literal *NewIntegerLiteral(int64_t val, const Location &location) {
        return new (arena_) Literal(val, location);
    }
    
    Literal *NewStringLiteral(const AstString *val, const Location &location) {
        return new (arena_) Literal(val, location);
    }
    
    Literal *NewStringLiteral(const char *s, size_t n,
                              const Location &location) {
        return new (arena_) Literal(NewString(s, n), location);
    }
    
    Literal *NewApproxLiteral(double val, const Location &location) {
        return new (arena_) Literal(val, location);
    }
    
    Literal *NewNullLiteral(const Location &location) {
        return new (arena_) Literal(Literal::NULL_VAL, location);
    }
    
    Literal *NewDefaultPlaceholderLiteral(const Location &location) {
        return new (arena_) Literal(Literal::DEFAULT_PLACEHOLDER, location);
    }
    
    Subquery *NewSubquery(bool match_all, Query *query,
                          const Location &location) {
        return new (arena_) Subquery(match_all, query, location);
    }
    
    Comparison *NewComparison(Expression *lhs, SQLOperator op,
                              Expression *rhs, const Location &location) {
        DCHECK(Comparison::IsComparisonOperator(op));
        return new (arena_) Comparison(op, lhs, rhs, location);
    }
    
    MultiExpression *NewMultiExpression(Expression *lhs, SQLOperator op,
                                        ExpressionList *rhs,
                                        const Location &location) {
        return new (arena_) MultiExpression(op, lhs, rhs, location);
    }
    
    MultiExpression *NewMultiExpression(Expression *lhs, SQLOperator op,
                                        Expression *rhs,
                                        const Location &location) {
        ExpressionList *stub = NewExpressionList(rhs);
        return NewMultiExpression(lhs, op, stub, location);
    }
    
    MultiExpression *NewMultiExpression(Expression *lhs, SQLOperator op,
                                        Expression *operand1,
                                        Expression *operand2,
                                        const Location &location) {
        ExpressionList *stub = NewExpressionList(operand1);
        stub->push_back(operand2);
        return NewMultiExpression(lhs, op, stub, location);
    }
    
    BinaryExpression *NewBinaryExpression(Expression *lhs, SQLOperator op,
                                          Expression *rhs,
                                          const Location &location) {
        DCHECK(BinaryExpression::IsBinaryOperator(op));
        return new (arena_) BinaryExpression(op, lhs, rhs, location);
    }
    
    UnaryExpression *NewUnaryExpression(SQLOperator op, Expression *operand,
                                        const Location &location) {
        DCHECK(UnaryExpression::IsUnaryOperator(op));
        return new (arena_) UnaryExpression(op, operand, location);
    }
    
    Assignment *NewAssignment(const AstString *name, Expression *rval,
                              const Location &location) {
        return new (arena_) Assignment(name, rval, location);
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Utils
    ////////////////////////////////////////////////////////////////////////////
    ShowTables *NewShowTables() { return new (arena_) ShowTables(); }
    
    AlterTableSpecList *NewAlterTableSpecList(AlterTableSpec *spec) {
        return NewList(spec);
    }
    
    ColumnDefinitionList *NewColumnDefinitionList(ColumnDefinition *def) {
        return NewList(def);
    }
    
    RowValuesList *NewRowValuesList(ExpressionList *values) {
        return NewList(values);
    }
    
    AssignmentList *NewAssignmentList(Assignment *a) { return NewList(a); }
    
    ProjectionColumnList *NewProjectionColumnList(ProjectionColumn *col) {
        return NewList(col);
    }
    
    ExpressionList *NewExpressionList(Expression *expr) {
        return NewList(expr);
    }
    
    NameList *NewNameList(const AstString *name) { return NewList(name); }
    
    const AstString *NewString(const char *s, size_t n) {
        return AstString::New(arena_, s, n);
    }
    
    const AstString *NewString(const char *s) {
        return AstString::New(arena_, s);
    }
    
    const AstString *Format(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        std::string str(base::Slice::Vsprintf(fmt, ap));
        va_end(ap);
        return AstString::New(arena_, str);
    }
    
    DEF_PTR_GETTER_NOTNULL(base::Arena, arena);
private:
    template<class T>
    base::ArenaVector<T> *NewList(T first) {
        base::ArenaVector<T> *list = new (arena_) base::ArenaVector<T>(arena_);
        list->reserve(8);
        if (first) {
            list->push_back(first);
        }
        return list;
    }
    
    base::Arena *const arena_;
}; // class AstFactory
    
} // namespace ast
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_AST_FACTORY_H_
