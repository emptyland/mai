#pragma once
#ifndef MAI_LANG_AST_H_
#define MAI_LANG_AST_H_

#include "lang/syntax.h"
#include "base/arena-utils.h"

namespace mai {

namespace lang {

#define DECLARE_ALL_AST(V) \
    V(FileUnit) \
    V(ImportStatement) \
    V(VariableDeclaration)

#define DEFINE_DECLARE(name) class name;
DECLARE_ALL_AST(DEFINE_DECLARE)
#undef DEFINE_DECLARE

struct ASTVisitorResult {
    
};

class ASTVisitor {
public:
    using Result = ASTVisitorResult;
    
    ASTVisitor() {}
    virtual ~ASTVisitor() {}
    
#define DEFINE_METHOD(name) virtual Result Visit##name(name *) = 0;
    DECLARE_ALL_AST(DEFINE_METHOD)
#undef DEFINE_METHOD
}; // class ASTVisitor

class ASTNode {
public:
    enum Kind {
#define DEFINE_ENUM(name) k##name,
        DECLARE_ALL_AST(DEFINE_ENUM)
#undef DEFINE_ENUM
    }; // enum Kind
    
    DEF_VAL_GETTER(int, position);
    DEF_VAL_GETTER(Kind, kind);
    
#define DEFINE_TYPE_CHECKER(name) \
    inline bool Is##name() const { return kind() == k##name; } \
    inline name *As##name() { return !Is##name() ? nullptr : reinterpret_cast<name *>(this); } \
    inline const name *As##name() const { return !Is##name() ? nullptr : reinterpret_cast<const name *>(this); }
    
    DECLARE_ALL_AST(DEFINE_TYPE_CHECKER)
    
#undef DEFINE_TYPE_CHECKER
    

    ALWAYS_INLINE
    void *operator new (size_t size, base::Arena *arena) { return arena->Allocate(size); }

    virtual ASTVisitor::Result Accept(ASTVisitor *visitor) = 0;

    DISALLOW_IMPLICIT_CONSTRUCTORS(ASTNode);
protected:
    ASTNode(int position, Kind kind): position_(position), kind_(kind) {}
    
    int position_; // Source file position
    Kind kind_; // Kind of AST node
}; // class ASTNode

#define DEFINE_AST_NODE(name) \
    ASTVisitor::Result Accept(ASTVisitor *visitor) override { \
        return visitor->Visit##name(this); \
    } \
    DISALLOW_IMPLICIT_CONSTRUCTORS(name)

class Declaration;
class Definition;
class Statement;
class Expression;

class FileUnit : public ASTNode {
public:
    FileUnit(base::Arena *arena)
        : ASTNode(0, kFileUnit)
        , import_packages_(arena)
        , global_variables_(arena)
        , functions_(arena)
        , source_location_map_(arena) {}
    
    DEF_PTR_PROP_RW(const ASTString, file_name);
    DEF_PTR_PROP_RW(const ASTString, package_name);
    DEF_VAL_PROP_RM(base::ArenaVector<ImportStatement *>, import_packages);
    
    void InsertImportStatement(ImportStatement *stmt) { import_packages_.push_back(stmt); }
    void InsertGlobalVariable(Declaration *decl) { global_variables_.push_back(decl); }
    void InsertFunction(Definition *def) { functions_.push_back(def); }

    int InsertSourceLocation(const SourceLocation loc) {
        int position = static_cast<int>(source_location_map_.size());
        source_location_map_.push_back(loc);
        return position;
    }
    DEFINE_AST_NODE(FileUnit);
private:
    const ASTString *file_name_ = nullptr;
    const ASTString *package_name_ = nullptr;
    base::ArenaVector<ImportStatement *> import_packages_;
    base::ArenaVector<Declaration *> global_variables_;
    base::ArenaVector<Definition *> functions_;
    base::ArenaVector<SourceLocation> source_location_map_;
}; // class FileUnit


class Statement : public ASTNode {
public:
    
protected:
    Statement(int position, Kind kind): ASTNode(position, kind) {}
}; // class Statement


class ImportStatement : public Statement {
public:
    ImportStatement(int position, const ASTString *alias, const ASTString *original_package_name)
        : Statement(position, kImportStatement)
        , alias_(alias)
        , original_package_name_(original_package_name) {}
    
    DEF_PTR_GETTER(const ASTString, alias);
    DEF_PTR_GETTER(const ASTString, original_package_name);
    
    DEFINE_AST_NODE(ImportStatement);
private:
    const ASTString *alias_;
    const ASTString *original_package_name_;
}; // class ImportStatement


class Expression : public ASTNode {
protected:
    Expression(int position, Kind kind): ASTNode(position, kind) {}
}; // // class Statement


class Symbolize : public Statement {
public:
    Symbolize(int position, Kind kind, const ASTString *identifier)
        : Statement(position, kind)
        , identifier_(identifier) {}
    
    DEF_PTR_GETTER(const ASTString, identifier);
protected:
    const ASTString *identifier_;
}; // class Symbolize


class Declaration : public Symbolize {
protected:
    Declaration(int position, Kind kind, const ASTString *identifier)
        : Symbolize(position, kind, identifier) {}
}; // class Declaration


class Definition : public Symbolize {
protected:
    Definition(int position, Kind kind, const ASTString *identifier)
        : Symbolize(position, kind, identifier) {}
}; // class Declaration


// var identifier = expression
// val identifier = expression
class VariableDeclaration : public Declaration {
public:
    enum Kind {
        VAL,
        VAR,
    };

    VariableDeclaration(int position, Kind kind, const ASTString *identifier, Expression *initializer)
        : Declaration(position, kVariableDeclaration, identifier)
        , kind_(kind)
        , initializer_(initializer) {}
private:
    Kind kind_;
    Expression *initializer_;
}; // class VariableDeclaration


class UnaryExpression : public Expression {
public:
    
private:
    Operator op_;
    Expression *operand_;
}; // class UnaryExpression


} // namespace lang

} // namespace mai

#endif // MAI_LANG_AST_H_
