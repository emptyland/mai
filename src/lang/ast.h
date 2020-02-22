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
    V(FunctionDefinition) \
    V(ClassDefinition) \
    V(ClassImplementsBlock) \
    V(AssignmentStatement) \
    V(BreakableStatement) \
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
    V(LambdaLiteral) \
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
class Class;

struct ASTVisitorResult {
    int kind;
    union {
        int index;
        HValue *hval;
        Class *clazz;
        TypeSign *sign;
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

class PartialASTVisitor : public ASTVisitor {
public:
    PartialASTVisitor() {}
    ~PartialASTVisitor() override {}
    
#define DEFINE_METHOD(name) Result Visit##name(name *) override { TODO(); return Result{}; }
    DECLARE_ALL_AST(DEFINE_METHOD)
#undef DEFINE_METHOD
}; // class PartialASTVisito

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
        , definitions_(arena)
        , implements_(arena)
        , source_location_map_(arena) {}
    
    DEF_PTR_PROP_RW(const ASTString, file_name);
    DEF_PTR_PROP_RW(const ASTString, package_name);
    DEF_ARENA_VECTOR_GETTER(ImportStatement *, import_package);
    DEF_ARENA_VECTOR_GETTER(Declaration *, global_variable);
    DEF_ARENA_VECTOR_GETTER(Definition *, definition);
    DEF_ARENA_VECTOR_GETTER(ClassImplementsBlock *, implement);
    
    std::string GetPathName(const std::string &ext_name) const {
        std::string buf(file_name()->ToString());
        if (!ext_name.empty() && buf.length() - buf.rfind(ext_name) == ext_name.length()) {
            buf = buf.substr(buf.rfind(ext_name));
        }
        for (size_t i = 0; i < buf.size(); i++) {
            if (buf[i] == '/' || buf[i] == '\\') {
                buf[i] = '.';
            }
        }
        return buf;
    }
    
    void InsertImportStatement(ImportStatement *stmt) { import_packages_.push_back(stmt); }
    void InsertGlobalVariable(Declaration *decl) { global_variables_.push_back(decl); }
    void InsertDefinition(Definition *def) { definitions_.push_back(def); }
    void InsertImplement(ClassImplementsBlock *impl) { implements_.push_back(impl); }

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
    base::ArenaVector<Definition *> definitions_;
    base::ArenaVector<ClassImplementsBlock *> implements_;
    base::ArenaVector<SourceLocation> source_location_map_;
}; // class FileUnit

class FunctionPrototype;

class TypeSign : public ASTNode {
public:
    constexpr TypeSign(int position, int kind): ASTNode(position, kTypeSign), kind_(kind), symbol_(nullptr) {}
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
    DEF_PTR_GETTER(ClassDefinition, clazz);

    size_t parameters_size() const { return parameters_->size(); }
    TypeSign *parameter(size_t i) const {
        DCHECK_LT(i, parameters_size());
        return parameters_->at(i);
    }
    
    bool Convertible(TypeSign *type) const;
    
    DEFINE_AST_NODE(TypeSign);
private:
    const int kind_;
    union {
        const ASTString *symbol_;
        ClassDefinition *clazz_;
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
    DEF_ARENA_VECTOR_GETTER(Parameter, parameter);
    
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
    TypeSign *return_type_ = nullptr;
    bool vargs_ = false;
}; // class FunctionPrototype

class Statement : public ASTNode {
public:
    
protected:
    Statement(int position, Kind kind): ASTNode(position, kind) {}
}; // class Statement


class ImportStatement : public Statement {
public:
    ImportStatement(int position, const ASTString *alias, const ASTString *original_path)
        : Statement(position, kImportStatement)
        , alias_(alias)
        , original_path_(original_path) {}
    
    DEF_PTR_GETTER(const ASTString, alias);
    DEF_PTR_GETTER(const ASTString, original_path);
    
    DEFINE_AST_NODE(ImportStatement);
private:
    const ASTString *alias_;
    const ASTString *original_path_;
}; // class ImportStatement


class Expression : public Statement {
public:
    virtual Operator GetOperator() const { return Operators::NOT_OPERATOR; }

    bool IsLValue() const {
        return IsIdentifier() || IsDotExpression() || IsIndexExpression();
    }
protected:
    Expression(int position, Kind kind): Statement(position, kind) {}
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
    DEF_PTR_PROP_RW(TypeSign, type);
    DEF_PTR_PROP_RW(Expression, initializer);
    
    DEFINE_AST_NODE(VariableDeclaration);
private:
    Kind kind_;
    TypeSign *type_;
    Expression *initializer_;
}; // class VariableDeclaration


class FunctionDefinition : public Definition {
public:
    FunctionDefinition(int position, const ASTString *identifier, LambdaLiteral *body)
        : Definition(position, kFunctionDefinition, identifier)
        , native_(false)
        , body_(body) {}
    
    FunctionDefinition(int position, const ASTString *identifier, FunctionPrototype *prototype)
        : Definition(position, kFunctionDefinition, identifier)
        , native_(true)
        , prototype_(prototype) {}

    DEF_VAL_GETTER(bool, native);
    DEF_PTR_PROP_RW(LambdaLiteral, body);
    inline size_t parameters_size() const;
    inline FunctionPrototype::Parameter parameter(size_t i) const;
    inline TypeSign *return_type() const;
    inline FunctionPrototype *prototype() const;
    inline size_t statements_size() const;
    inline Statement *statement(size_t i) const;
    
    DEFINE_AST_NODE(FunctionDefinition);
private:
    bool native_;
    union {
        LambdaLiteral *body_;
        FunctionPrototype *prototype_;
    };
}; // class FunctionDefinition


class ClassDefinition : public Definition {
public:
    enum Access {
        kPublic,
        kProtected,
        kPrivate,
    }; // enum Access
    
    struct Field {
        Access access;
        bool in_constructor; // in constructor
        int  as_constructor; // index of constructor parameter
        VariableDeclaration *declaration;
    }; // struct Field
    
    struct Parameter {
        bool field_declaration;
        union {
            int as_field; // index of field
            FunctionPrototype::Parameter as_parameter;
        };
    }; // struct Field

    ClassDefinition(base::Arena *arena,
                    int position,
                    const ASTString *name,
                    base::ArenaVector<Parameter> &&constructor,
                    base::ArenaVector<Field> &&fields,
                    const ASTString *base_name,
                    base::ArenaVector<Expression *> &&arguments);

    DEF_ARENA_VECTOR_GETTER(Field, field);
    DEF_ARENA_VECTOR_GETTER(Parameter, parameter);
    DEF_ARENA_VECTOR_GETTER(Expression *, argument);
    DEF_ARENA_VECTOR_GETTER(FunctionDefinition *, method);
    DEF_PTR_PROP_RW(const ASTString, base_name);
    DEF_PTR_PROP_RW(ClassDefinition, base);
    
    bool InsertMethod(FunctionDefinition *method) {
        if (FindFieldOrNull(method->identifier()->ToSlice()) ||
            FindMethodOrNull(method->identifier()->ToSlice())) {
            return false;
        }
        methods_.push_back(method);
        named_methods_[method->identifier()->ToSlice()] = method;
        return true;
    }
    
    int MakeParameterLookupTable();
    
    int MakeFieldLookupTable() {
        for (size_t i = 0; i < fields_size(); i++) {
            const ASTString *name = field(i).declaration->identifier();
            if (auto iter = named_fields_.find(name->ToSlice()); iter != named_fields_.end()) {
                return -static_cast<int>(i + 1);
            }
            named_fields_[name->ToSlice()] = i;
        }
        return 0;
    }
    
    Field *FindFieldOrNull(std::string_view name) {
        auto iter = named_fields_.find(name);
        return iter == named_fields_.end() ? nullptr : &fields_[iter->second];
    }
    
    FunctionDefinition *FindMethodOrNull(std::string_view name) {
        auto iter = named_methods_.find(name);
        return iter == named_methods_.end() ? nullptr : iter->second;
    }

    DEFINE_AST_NODE(ClassDefinition);
private:
    base::ArenaVector<Parameter> parameters_;
    base::ArenaMap<std::string_view, size_t> named_parameters_;
    base::ArenaVector<Field> fields_;
    base::ArenaMap<std::string_view, size_t> named_fields_;
    const ASTString *base_name_;
    ClassDefinition *base_;
    base::ArenaVector<Expression *> arguments_;
    base::ArenaVector<FunctionDefinition *> methods_;
    base::ArenaMap<std::string_view, FunctionDefinition *> named_methods_;
}; // class ClassDefinition


class ClassImplementsBlock : public Statement {
public:
    ClassImplementsBlock(int position, const ASTString *prefix, const ASTString *name,
                         base::ArenaVector<FunctionDefinition *> &&methods)
        : Statement(position, kClassImplementsBlock)
        , prefix_(prefix)
        , name_(name)
        , methods_(methods) {}

    DEF_PTR_GETTER(const ASTString, prefix);
    DEF_PTR_GETTER(const ASTString, name);
    DEF_ARENA_VECTOR_GETTER(FunctionDefinition *, method);
    
    DEFINE_AST_NODE(ClassImplementsBlock);
private:
    const ASTString *prefix_;
    const ASTString *name_;
    base::ArenaVector<FunctionDefinition *> methods_;
}; // class ClassImplementsBlock


class AssignmentStatement : public Statement {
public:
    AssignmentStatement(int position, Operator assignment_op, Expression *lval, Expression *rval)
        : Statement(position, kAssignmentStatement)
        , assignment_op_(assignment_op)
        , lval_(lval)
        , rval_(rval) {
        DCHECK(lval->IsLValue());
    }
    
    DEF_VAL_GETTER(Operator, assignment_op);
    DEF_PTR_PROP_RW(Expression, lval);
    DEF_PTR_PROP_RW(Expression, rval);

    DEFINE_AST_NODE(AssignmentStatement);
private:
    Operator assignment_op_;
    Expression *lval_;
    Expression *rval_;
}; // class AssignmentStatement


class BreakableStatement : public Statement {
public:
    enum Control {
        RETURN,
        BREAK,
        CONTINUE,
    }; // enum Control
    
    BreakableStatement(int position, Control control, Expression *value = nullptr)
        : Statement(position, kBreakableStatement)
        , control_(control)
        , value_(value) {}
    
    bool IsReturn() const { return control_ == RETURN; }
    bool IsBreak() const { return control_ == BREAK; }
    bool IsContinue() const { return control_ == CONTINUE; }

    DEF_PTR_PROP_RW(Expression, value);

    DEFINE_AST_NODE(BreakableStatement);
private:
    Control control_;
    Expression *value_;
}; // class BreakableStatement


class Literal : public Expression {
public:
    DEF_PTR_PROP_RW(TypeSign, type);
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

class LambdaLiteral : public Literal {
public:
    LambdaLiteral(int position, TypeSign *type, base::ArenaVector<Statement *> &&statements)
        : Literal(position, kLambdaLiteral, type)
        , prototype_(DCHECK_NOTNULL(type->prototype()))
        , statements_(statements) {}

    DEF_PTR_GETTER(FunctionPrototype, prototype);
    DEF_ARENA_VECTOR_GETTER(Statement *, statement);

    DEFINE_AST_NODE(LambdaLiteral);
private:
    FunctionPrototype *prototype_;
    base::ArenaVector<Statement *> statements_;
}; // class LambdaLiteral

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
    
    DEF_ARENA_VECTOR_GETTER(Expression *, operand);
protected:
    MultiExpression(int position, Kind kind, base::ArenaVector<Expression *> &&parts)
        : Expression(position, kind)
        , operands_(parts) {}
    
    MultiExpression(base::Arena *arena, int position, Kind kind)
        : Expression(position, kind)
        , operands_(arena) {}

    base::ArenaVector<Expression *> operands_;
}; // class MultiExpression


class StringTemplateExpression : public MultiExpression {
public:
    StringTemplateExpression(int position, base::ArenaVector<Expression *> &&parts)
        : MultiExpression(position, kStringTemplateExpression, std::move(parts)) {}
        
    DEFINE_AST_NODE(StringTemplateExpression);
}; // class StringTemplateExpression



class CallExpression : public MultiExpression {
public:
    CallExpression(int position, Expression *callee, base::ArenaVector<Expression *> &&argv)
        : MultiExpression(position, kCallExpression, std::move(argv))
        , callee_(callee) {}
    
    DEF_PTR_PROP_RW(Expression, callee);
    
    DEFINE_AST_NODE(CallExpression);
private:
    Expression *callee_;
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


// array[type](reserve)
// array[type]{1,2,3}
// array{"ok", "no", "over"}
class ArrayInitializer : public MultiExpression {
public:
    ArrayInitializer(int position, bool mutable_container, TypeSign *element_type,
                     base::ArenaVector<Expression *> &&elements)
        : MultiExpression(position, kArrayInitializer, std::move(elements))
        , mutable_container_(mutable_container)
        , element_type_(element_type) {}
    ArrayInitializer(base::Arena *arena, int position, bool mutable_container,
                     TypeSign *element_type, Expression *reserve)
        : MultiExpression(arena, position, kArrayInitializer)
        , mutable_container_(mutable_container)
        , element_type_(element_type)
        , reserve_(reserve) {}

    DEF_VAL_GETTER(bool, mutable_container);
    DEF_PTR_PROP_RW(Expression, reserve);
    DEF_PTR_PROP_RW(TypeSign, element_type);
    
    DEFINE_AST_NODE(ArrayInitializer);
private:
    bool mutable_container_;
    Expression *reserve_; // reserve parameter
    TypeSign *element_type_;
}; // class ArrayInitializer

class MapInitializer : public MultiExpression {
public:
    MapInitializer(int position, bool mutable_container, TypeSign *key_type,
                   TypeSign *value_type, base::ArenaVector<Expression *> &&elements)
        : MultiExpression(position, kArrayInitializer, std::move(elements))
        , mutable_container_(mutable_container)
        , reserve_(nullptr)
        , load_factor_(nullptr)
        , key_type_(key_type)
        , value_type_(value_type) {
    #if defined(DEBUG) || defined(_DEBUG)
        for (auto pair : elements) {
            DCHECK(pair->IsPairExpression());
        }
    #endif
    }

    MapInitializer(base::Arena *arena, int position, bool mutable_container, TypeSign *key_type,
                   TypeSign *value_type, Expression *reserve, Expression *load_factor)
        : MultiExpression(arena, position, kArrayInitializer)
        , mutable_container_(mutable_container)
        , reserve_(reserve)
        , load_factor_(load_factor)
        , key_type_(key_type)
        , value_type_(value_type) {}

    DEF_VAL_GETTER(bool, mutable_container);
    DEF_PTR_PROP_RW(Expression, reserve);
    DEF_PTR_PROP_RW(Expression, load_factor);
    DEF_PTR_PROP_RW(TypeSign, key_type);
    DEF_PTR_PROP_RW(TypeSign, value_type);
    
    DEFINE_AST_NODE(MapInitializer);
private:
    bool mutable_container_;
    Expression *reserve_; // reserve parameter
    Expression *load_factor_; // factor for hash map
    TypeSign *key_type_;
    TypeSign *value_type_;
}; // class MapInitializer

inline size_t FunctionDefinition::parameters_size() const {
    return prototype()->parameters_size();
}

inline FunctionPrototype::Parameter FunctionDefinition::parameter(size_t i) const {
    return prototype()->parameter(i);
}

inline TypeSign *FunctionDefinition::return_type() const {
    return prototype()->return_type();
}

inline FunctionPrototype * FunctionDefinition::prototype() const {
    return native_ ? prototype_ : body_->prototype();
}

inline size_t FunctionDefinition::statements_size() const {
    return native_ ? 0 : body_->statements_size();
}

inline Statement *FunctionDefinition::statement(size_t i) const {
    return native_ ? nullptr : body_->statement(i);
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_AST_H_
