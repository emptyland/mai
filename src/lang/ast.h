#pragma once
#ifndef MAI_LANG_AST_H_
#define MAI_LANG_AST_H_

#include "lang/syntax.h"
#include "base/arena-utils.h"

namespace mai {

namespace lang {

#define DECLARE_ALL_AST(V) \
    V(TypeSign) \
    V(FileUnit) \
    V(ImportStatement) \
    V(VariableDeclaration) \
    V(StringLiteral) \
    V(BoolLiteral) \
    V(I8Literal) \
    V(U8Literal) \
    V(I16Literal) \
    V(U16Literal) \
    V(I32Literal) \
    V(U32Literal) \
    V(I64Literal) \
    V(U64Literal) \
    V(IntLiteral) \
    V(UIntLiteral) \
    V(F32Literal) \
    V(F64Literal) \
    V(NilLiteral) \
    V(Identifier) \
    V(UnaryExpression) \
    V(BinaryExpression) \
    V(DotExpression) \
    V(IndexExpression) \
    V(StringTemplateExpression) \
    V(CallExpression) \
    V(PairExpression) \
    V(ArrayInitializer) \
    V(MapInitializer)

#define DEFINE_DECLARE(name) class name;
DECLARE_ALL_AST(DEFINE_DECLARE)
#undef DEFINE_DECLARE

class HValue;

struct ASTVisitorResult {
    int kind;
    union {
        int index;
        HValue *hval;
    };
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
    constexpr ASTNode(int position, Kind kind): position_(position), kind_(kind) {}
    
    const int position_; // Source file position
    const Kind kind_; // Kind of AST node
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
    
    const SourceLocation &FindSourceLocation(const ASTNode *ast) const {
        DCHECK(ast != nullptr);
        DCHECK(ast->position() >= 0 && ast->position() < source_location_map_.size());
        return source_location_map_[ast->position()];
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

class FunctionPrototype;

class TypeSign : public ASTNode {
public:
    TypeSign(int position, int kind): ASTNode(position, kTypeSign), kind_(kind) {}
    TypeSign(int position, const ASTString *symbol);
    TypeSign(int position, FunctionPrototype *prototype);

    TypeSign(base::Arena *arena, int position, int kind, TypeSign *param)
        : ASTNode(position, kTypeSign)
        , kind_(kind)
        , parameters_(new (arena) base::ArenaVector<TypeSign *>(arena)) {
        InsertParameter(param);
    }

    TypeSign(base::Arena *arena, int position, int kind, TypeSign *param0, TypeSign *param1)
        : ASTNode(position, kTypeSign)
        , kind_(kind)
        , parameters_(new (arena) base::ArenaVector<TypeSign *>(arena)) {
        InsertParameter(param0);
        InsertParameter(param1);
    }
    
    void InsertParameter(TypeSign *type) { DCHECK_NOTNULL(parameters_)->push_back(type); }
    
    int id() const { return kind_; }
    DEF_PTR_GETTER(const ASTString, symbol);
    DEF_PTR_GETTER(FunctionPrototype, prototype);
    
    DEFINE_AST_NODE(TypeSign);
private:
    const int kind_;
    union {
        const ASTString *symbol_;
        base::ArenaVector<TypeSign *> *parameters_;
        FunctionPrototype *prototype_;
    };
}; // class TypeLink

class FunctionPrototype {
public:
    struct Parameter {
        const ASTString *name;
        TypeSign *type;
    };
    FunctionPrototype(base::Arena *arena): parameters_(arena) {}
    
    DEF_PTR_PROP_RW(TypeSign, return_type);
    DEF_VAL_PROP_RW(bool, vargs);
    
    size_t parameters_size() const { return parameters_.size(); }
    
    const Parameter &parameter(size_t i) const {
        DCHECK_LT(i, parameters_.size());
        return parameters_[i];
    }
    
    Parameter *mutable_parameter(size_t i) {
        DCHECK_LT(i, parameters_.size());
        return &parameters_[i];
    }
    
    ALWAYS_INLINE
    void *operator new (size_t size, base::Arena *arena) { return arena->Allocate(size); }
    
    void InsertParameter(const ASTString *name, TypeSign *type) {
        parameters_.push_back({name, type});
    }
private:
    base::ArenaVector<Parameter> parameters_;
    TypeSign *return_type_;
    bool vargs_ = false;
}; // class FunctionPrototype

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
public:
    virtual Operator GetOperator() const { return Operators::NOT_BINARY; }

    bool IsLValue() const {
        return IsIdentifier() || IsDotExpression() || IsIndexExpression();
    }
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

    VariableDeclaration(int position, Kind kind, const ASTString *identifier, TypeSign *type,
                        Expression *initializer)
        : Declaration(position, kVariableDeclaration, identifier)
        , kind_(kind)
        , type_(type)
        , initializer_(initializer) {}
    
    DEF_VAL_GETTER(Kind, kind);
    DEF_PTR_GETTER(TypeSign, type);
    DEF_PTR_GETTER(Expression, initializer);
    
    DEFINE_AST_NODE(VariableDeclaration);
private:
    Kind kind_;
    TypeSign *type_;
    Expression *initializer_;
}; // class VariableDeclaration



class Literal : public Expression {
public:
    DEF_PTR_GETTER(TypeSign, type);
protected:
    Literal(int position, Kind kind, TypeSign *type): Expression(position, kind), type_(type) {}
    TypeSign *type_;
}; // class Literal


template<class T>
class LiteralBase : public Literal {
public:
    DEF_VAL_GETTER(T, value);
protected:
    LiteralBase(int position, Kind kind, TypeSign *type, T value)
        : Literal(position, kind, type)
        , value_(value) {}
    T value_;
}; // class LiteralBase

#define DEFINE_SIMPLE_LITERAL(name, value_type) \
    class name##Literal : public LiteralBase< value_type > { \
    public: \
        name##Literal(int position, TypeSign *type, value_type value) \
            : LiteralBase(position, k##name##Literal, type, value) {} \
        DEFINE_AST_NODE(name##Literal); \
    }

DEFINE_SIMPLE_LITERAL(Bool, bool);
DEFINE_SIMPLE_LITERAL(I8, int32_t);
DEFINE_SIMPLE_LITERAL(U8, uint32_t);
DEFINE_SIMPLE_LITERAL(I16, int32_t);
DEFINE_SIMPLE_LITERAL(U16, uint32_t);
DEFINE_SIMPLE_LITERAL(I32, int32_t);
DEFINE_SIMPLE_LITERAL(U32, uint32_t);
DEFINE_SIMPLE_LITERAL(I64, int64_t);
DEFINE_SIMPLE_LITERAL(U64, uint64_t);
DEFINE_SIMPLE_LITERAL(Int, int32_t);
DEFINE_SIMPLE_LITERAL(UInt, uint32_t);
DEFINE_SIMPLE_LITERAL(F32, float);
DEFINE_SIMPLE_LITERAL(F64, double);
DEFINE_SIMPLE_LITERAL(String, const ASTString *);
DEFINE_SIMPLE_LITERAL(Nil, void *);

#undef DEFINE_SIMPLE_LITERAL

class Identifier : public Expression {
public:
    Identifier(int position, const ASTString *name)
        : Expression(position, kIdentifier)
        , name_(name) {}
    
    DEF_PTR_GETTER(const ASTString, name);
    
    DEFINE_AST_NODE(Identifier);
private:
    const ASTString *name_;
}; // class Identifier

class UnaryExpression : public Expression {
public:
    UnaryExpression(int position, Operator op, Expression *operand)
        : Expression(position, kUnaryExpression)
        , op_(op)
        , operand_(operand) {}

    DEF_VAL_GETTER(Operator, op);
    DEF_PTR_PROP_RW(Expression, operand);
    
    Operator GetOperator() const override { return op(); }

    DEFINE_AST_NODE(UnaryExpression);
private:
    Operator op_;
    Expression *operand_;
}; // class UnaryExpression


class BinaryExpression : public Expression {
public:
    BinaryExpression(int position, Operator op, Expression *lhs, Expression *rhs)
        : Expression(position, kBinaryExpression)
        , op_(op)
        , lhs_(lhs)
        , rhs_(rhs) {}
    
    DEF_VAL_GETTER(Operator, op);
    DEF_PTR_PROP_RW(Expression, lhs);
    DEF_PTR_PROP_RW(Expression, rhs);
    
    Operator GetOperator() const override { return op(); }
    
    DEFINE_AST_NODE(BinaryExpression);
private:
    Operator op_;
    Expression *lhs_, *rhs_;
}; // class BinaryExpression


class DotExpression : public Expression {
public:
    DotExpression(int position, Expression *primary, const ASTString *rhs)
        : Expression(position, kDotExpression)
        , primary_(primary)
        , rhs_(rhs) {}
    
    DEF_PTR_PROP_RW(Expression, primary);
    DEF_PTR_PROP_RW(const ASTString, rhs);

    DEFINE_AST_NODE(DotExpression);
private:
    Expression *primary_;
    const ASTString *rhs_;
}; // class DotExpression


class IndexExpression : public Expression {
public:
    IndexExpression(int position, Expression *primary, Expression *index)
        : Expression(position, kIndexExpression)
        , primary_(primary)
        , index_(index) {}

    DEF_PTR_PROP_RW(Expression, primary);
    DEF_PTR_PROP_RW(Expression, index);
    
    DEFINE_AST_NODE(IndexExpression);
private:
    Expression *primary_;
    Expression *index_;
}; // class IndexExpression


class MultiExpression : public Expression {
public:
    void InsertOperand(Expression *expr) { operands_.push_back(expr); }
    
protected:
    MultiExpression(base::Arena *arena, int position, Kind kind, const std::vector<Expression *> parts)
        : Expression(position, kind)
        , operands_(arena) {
        for (auto expr : parts) { InsertOperand(expr); }
    }
    
    base::ArenaVector<Expression *> operands_;
}; // class MultiExpression


class StringTemplateExpression : public MultiExpression {
public:
    StringTemplateExpression(base::Arena *arena, int position, const std::vector<Expression *> parts)
        : MultiExpression(arena, position, kStringTemplateExpression, parts) {}
        
    DEFINE_AST_NODE(StringTemplateExpression);
}; // class StringTemplateExpression



class CallExpression : public MultiExpression {
public:
    CallExpression(base::Arena *arena, int position, const std::vector<Expression *> argv)
        : MultiExpression(arena, position, kCallExpression, argv) {}
    
    DEFINE_AST_NODE(CallExpression);
}; // class CallExpression


class PairExpression : public Expression {
public:
    PairExpression(int position, Expression *value)
        : Expression(position, kPairExpression)
        , addition_key_(true)
        , key_(nullptr)
        , value_(value) {}

    PairExpression(int position, Expression *key, Expression *value)
        : Expression(position, kPairExpression)
        , addition_key_(false)
        , key_(key)
        , value_(value) {}

    DEF_VAL_GETTER(bool, addition_key);
    DEF_PTR_PROP_RW(Expression, key);
    DEF_PTR_PROP_RW(Expression, value);
    
    DEFINE_AST_NODE(PairExpression);
private:
    bool addition_key_;
    Expression *key_;
    Expression *value_;
}; // class PairExpression

} // namespace lang

} // namespace mai

#endif // MAI_LANG_AST_H_
