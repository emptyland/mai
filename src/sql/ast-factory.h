#ifndef MAI_SQL_AST_FACTORY_H_
#define MAI_SQL_AST_FACTORY_H_

#include "sql/ast.h"
#include "base/slice.h"
#include <stdarg.h>

namespace mai {
    
namespace sql {
    
class AstFactory final {
public:
    AstFactory(base::Arena *arena) : arena_(arena) {}
    ~AstFactory() {}
    
    Block *NewBlock() { return new (arena_) Block(arena_); }

    ////////////////////////////////////////////////////////////////////////////
    // DDL
    ////////////////////////////////////////////////////////////////////////////
    CreateTable *NewCreateTable(const AstString *schema_name,
                                ColumnDefinitionList *cols) {
        return new (arena_) CreateTable(schema_name, cols);
    }
    
    AlterTable *NewAlterTable(const AstString *table_name,
                              AlterTableSpecList *spces) {
        return new (arena_) AlterTable(table_name, spces);
    }
    
    AlterTableSpecList *NewAlterTableSpecList(AlterTableSpec *spec) {
        auto specs =  new (arena_) AlterTableSpecList(arena_);
        specs->reserve(8);
        if (spec) {
            specs->push_back(spec);
        }
        return specs;
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

    DropTable *NewDropTable(const AstString *schema_name) {
        return new (arena_) DropTable(schema_name);
    }
    
    ColumnDefinition *NewColumnDefinition(const AstString *name,
                                            TypeDefinition *type,
                                            bool is_not_null,
                                            bool auto_increment,
                                            SQLKeyType key) {
        return new (arena_) ColumnDefinition(name, type, is_not_null,
                                              auto_increment, key);
    }
    
    ColumnDefinitionList *
    NewColumnDefinitionList(ColumnDefinition *def = nullptr) {
        auto defs = new (arena_) ColumnDefinitionList(arena_);
        defs->reserve(8);
        if (def) {
            defs->push_back(def);
        }
        return defs;
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

    ////////////////////////////////////////////////////////////////////////////
    // Expressions
    ////////////////////////////////////////////////////////////////////////////
    ProjectionColumn *NewProjectionColumn(Expression *expr,
                                          const AstString *alias) {
        return new (arena_) ProjectionColumn(alias, expr);
    }
    
    Identifier *NewIdentifier(const AstString *prefix, const AstString *name) {
        return new (arena_) Identifier(prefix, name);
    }
    
    Literal *NewIntegerLiteral(int64_t val) {
        return new (arena_) Literal(val);
    }
    
    Literal *NewStringLiteral(const AstString *val) {
        return new (arena_) Literal(val);
    }
    
    Literal *NewStringLiteral(const char *s, size_t n) {
        return new (arena_) Literal(NewString(s, n));
    }
    
    Literal *NewApproxLiteral(double val) {
        return new (arena_) Literal(val);
    }
    
    Comparison *NewComparison(Expression *lhs, SQLOperator op,
                              Expression *rhs) {
        DCHECK(Comparison::IsComparisonOperator(op));
        return new (arena_) Comparison(op, lhs, rhs);
    }
    
    BinaryExpression *NewBinaryExpression(Expression *lhs, SQLOperator op,
                                          Expression *rhs) {
        DCHECK(BinaryExpression::IsBinaryOperator(op));
        return new (arena_) BinaryExpression(op, lhs, rhs);
    }
    
    UnaryExpression *NewUnaryExpression(SQLOperator op, Expression *operand) {
        DCHECK(UnaryExpression::IsUnaryOperator(op));
        return new (arena_) UnaryExpression(op, operand);
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Utils
    ////////////////////////////////////////////////////////////////////////////
    ShowTables *NewShowTables() { return new (arena_) ShowTables(); }
    
    ProjectionColumnList *NewProjectionColumnList(ProjectionColumn *col) {
        ProjectionColumnList *cols = new (arena_) ProjectionColumnList(arena_);
        cols->reserve(8);
        if (col) {
            cols->push_back(col);
        }
        return cols;
    }
    
    ExpressionList *NewExpressionList(Expression *expr) {
        ExpressionList *exprs = new (arena_) ExpressionList(arena_);
        exprs->reserve(8);
        if (expr) {
            exprs->push_back(expr);
        }
        return exprs;
    }
    
    NameList *NewNameList(const AstString *name) {
        NameList *names = new (arena_) NameList(arena_);
        names->reserve(8);
        if (name) {
            names->push_back(name);
        }
        return names;
    }
    
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
    base::Arena *const arena_;
}; // class AstFactory
    
} // namespace sql
    
} // namespace mai


#endif // MAI_SQL_AST_FACTORY_H_
