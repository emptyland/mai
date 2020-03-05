#pragma once
#ifndef MAI_LANG_AST_H_
#define MAI_LANG_AST_H_

#include "lang/syntax.h"
#include "lang/mm.h"
#include "lang/type-defs.h"
#include "base/arena-utils.h"

namespace mai {

namespace lang {

#define DECLARE_ALL_AST(V) \
    V(TypeSign) \
    V(FileUnit) \
    V(StatementBlock) \
    V(ImportStatement) \
    V(WhileLoop) \
    V(VariableDeclaration) \
    V(FunctionDefinition) \
    V(ClassDefinition) \
    V(ObjectDefinition) \
    V(InterfaceDefinition) \
    V(TryCatchFinallyBlock) \
    V(ClassImplementsBlock) \
    V(AssignmentStatement) \
    V(BreakableStatement) \
    V(IfExpression) \
    V(TypeCastExpression) \
    V(TypeTestExpression) \
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
        struct {
            int index;
            int type;
        } bundle;
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
    inline name *As##name() { return !this || !Is##name() ? nullptr : reinterpret_cast<name *>(this); } \
    inline const name *As##name() const { return !this || !Is##name() ? nullptr : reinterpret_cast<const name *>(this); }
    
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
class Circulation;
class StructureDefinition;
class PartialInterfaceDefinition;

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
    
    std::string GetPathName(size_t prefix, const std::string &ext_name) const {
        std::string buf(file_name()->ToString());
        if (!ext_name.empty() && buf.length() - buf.rfind(ext_name) == ext_name.length()) {
            buf = buf.substr(prefix);
            buf = buf.substr(0, buf.length() - ext_name.length());
        }
        for (size_t i = 0; i < buf.size(); i++) {
            if (buf[i] == '/' || buf[i] == '\\') {
                buf[i] = '.';
            }
        }
        return buf.substr(0, buf.rfind('.'));
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
    constexpr TypeSign(int position, int kind)
        : ASTNode(position, kTypeSign), kind_(kind), prefix_(nullptr) {}
    TypeSign(int position, const ASTString *prefix, const ASTString *name);
    TypeSign(int position, FunctionPrototype *prototype);
    TypeSign(int position, ClassDefinition *clazz);
    TypeSign(int position, ObjectDefinition *object);
    TypeSign(int position, InterfaceDefinition *interface);
    TypeSign(int position, int kind, Definition *def);

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
    void set_id(int id) { kind_ = id; }
    DEF_PTR_GETTER(const ASTString, prefix);
    DEF_PTR_GETTER(const ASTString, name);
    DEF_PTR_GETTER(FunctionPrototype, prototype);
    DEF_PTR_PROP_RW(ClassDefinition, clazz);
    DEF_PTR_PROP_RW(ObjectDefinition, object);
    DEF_PTR_PROP_RW(InterfaceDefinition, interface);
    
    bool IsNumber() const { return IsIntegral() || IsFloating(); }
    bool IsIntegral() const { return IsSignedIntegral() || IsUnsignedIntegral(); }
    bool IsSignedIntegral() const;
    bool IsUnsignedIntegral() const;
    bool IsFloating() const;
    
#if defined(DEBUG) || defined(_DEBUG)
    StructureDefinition *structure() const;
    Definition *definition() const;
#else
    DEF_PTR_GETTER(StructureDefinition, structure);
    DEF_PTR_GETTER(Definition, definition);
#endif // defined(DEBUG) || defined(_DEBUG)

    size_t parameters_size() const { return parameters_->size(); }
    TypeSign *parameter(size_t i) const {
        DCHECK_LT(i, parameters_size());
        return parameters_->at(i);
    }

    bool Convertible(TypeSign *type) const;

    std::string ToSymbolString() const;
    
    BuiltinType ToBuiltinType() const;
    
    size_t GetReferenceSize() const;
    
    static BuiltinType GetArrayBuiltinType(bool is_mutable, TypeSign *element);
    static BuiltinType GetMapBuiltinType(bool is_mutable, TypeSign *key);
    
    int FindNumberCastHint(std::string_view name) const;
    
    std::string ToString() const;
    
    TypeSign *Clone(base::Arena *arena) const {
        TypeSign *copied = new (arena) TypeSign(position(), kind_);
        copied->data_ = data_;
        copied->name_ = name_;
        return copied;
    }

    DEFINE_AST_NODE(TypeSign);
private:
    // kind:
    // Token::kIdentifier                -> prefix_, name_
    // Token::kRef                       -> definition_
    // Token::kClass                     -> clazz_
    // Token::kObject                    -> object_
    // Token::kInterface                 -> interface_
    // Token::kChannel                   -> parameters_[0]
    // Token::kArray/Token::MutableArray -> parameters_[0]
    // Token::kMap/Token::MutableMap     -> parameters_[0], parameters_[1]
    // Token::kPair                      -> parameters_[0], parameters_[1]
    // Token::kFun                       -> prototype_
    // Others...
    int kind_;
    union {
        const ASTString *prefix_;
        InterfaceDefinition *interface_;
        StructureDefinition *structure_;
        Definition *definition_;
        ObjectDefinition *object_;
        ClassDefinition *clazz_;
        base::ArenaVector<TypeSign *> *parameters_;
        FunctionPrototype *prototype_;
        void *data_;
    };
    const ASTString *name_ = nullptr;
}; // class TypeLink

class FunctionPrototype {
public:
    struct Parameter {
        const ASTString *name;
        TypeSign *type;
    };
    FunctionPrototype(base::Arena *arena): parameters_(arena) {}
    
    DEF_PTR_PROP_RW(TypeSign, return_type);
    DEF_PTR_PROP_RW(StructureDefinition, owner);
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
    
    bool IsAccept(FunctionPrototype *rhs) const {
        if (!return_type()->Convertible(rhs->return_type()) ||
            parameters_size() != rhs->parameters_size() || vargs() != rhs->vargs()) {
            return false;
        }
        for (size_t i = 0; i < parameters_size(); i++) {
            if (!parameter(i).type->Convertible(rhs->parameter(i).type)) {
                return false;
            }
        }
        return true;
    }
    
    std::string ToString() const;
private:
    base::ArenaVector<Parameter> parameters_;
    TypeSign *return_type_ = nullptr;
    StructureDefinition *owner_ = nullptr;
    bool vargs_ = false;
}; // class FunctionPrototype

class Statement : public ASTNode {
public:
    virtual bool IsExpression() const { return false; }
    
    Expression *AsExpression() {
        return !IsExpression() ? nullptr : reinterpret_cast<Expression *>(this);
    }
    
protected:
    Statement(int position, Kind kind): ASTNode(position, kind) {}
}; // class Statement


class StatementBlock : public Statement {
public:
    StatementBlock(int position, base::ArenaVector<Statement *> &&statements)
        : StatementBlock(position, kStatementBlock, std::move(statements)) {}
    
    DEF_ARENA_VECTOR_GETTER(Statement *, statement);
    DEF_PTR_PROP_RW(Statement, owner);
    
    DEFINE_AST_NODE(StatementBlock);
protected:
    StatementBlock(int position, Kind kind, base::ArenaVector<Statement *> &&statements)
        : Statement(position, kind)
        , statements_(std::move(statements)) {}
    
    base::ArenaVector<Statement *> statements_;
    Statement *owner_ = nullptr;
}; // class StatementBlock


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


class Circulation : public StatementBlock {
protected:
    Circulation(int position, Kind kind, base::ArenaVector<Statement *> &&statements)
        : StatementBlock(position, kind, std::move(statements)) {}
}; // class Circulation


class WhileLoop : public Circulation {
public:
    WhileLoop(int position, Expression *condition, base::ArenaVector<Statement *> &&statements)
        : Circulation(position, kWhileLoop, std::move(statements))
        , condition_(DCHECK_NOTNULL(condition)) {}
    
    DEF_PTR_PROP_RW(Expression, condition);
    
    DEFINE_AST_NODE(WhileLoop);
private:
    Expression *condition_;
}; // class WhileLoop


class Expression : public Statement {
public:
    bool IsExpression() const override { return true; }
    
    bool IsLValue() const {
        return IsIdentifier() || IsDotExpression() || IsIndexExpression();
    }
    
    bool IsLiteral() const {
        return IsBoolLiteral() || IsI8Literal() || IsU8Literal() || IsI16Literal() ||
               IsU16Literal() || IsI32Literal() || IsU32Literal() || IsI64Literal() ||
               IsU64Literal() || IsIntLiteral() || IsUIntLiteral() || IsLambdaLiteral() ||
               IsStringLiteral();
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
    DEF_PTR_PROP_RW(FileUnit, file_unit);
    DEF_VAL_PROP_RW(int, location);
protected:
    const ASTString *identifier_;
    FileUnit *file_unit_ = nullptr; // Only for global symbol
    int location_ = -1; // Compiler allocated location address
}; // class Symbolize


class Declaration : public Symbolize {
protected:
    Declaration(int position, Kind kind, const ASTString *identifier)
        : Symbolize(position, kind, identifier) {}
}; // class Declaration


class Definition : public Symbolize {
public:
    PartialInterfaceDefinition *AsPartialInterfaceDefinition() {
        return IsInterfaceDefinition() || IsClassDefinition() || IsObjectDefinition() ?
               reinterpret_cast<PartialInterfaceDefinition *>(this) : nullptr;
    }
    
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
        PARAMETER,
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
    enum Type {
        NATIVE,
        DECLARE, // Only in interface
        LITERAL,
    };
    
    FunctionDefinition(int position, const ASTString *identifier, LambdaLiteral *body)
        : Definition(position, kFunctionDefinition, identifier)
        , type_(LITERAL)
        , body_(body) {}
    
    FunctionDefinition(int position, bool native, const ASTString *identifier, FunctionPrototype *prototype)
        : Definition(position, kFunctionDefinition, identifier)
        , type_(native ? NATIVE : DECLARE)
        , prototype_(prototype) {}

    bool is_native() const { return type_ == NATIVE; }
    bool is_declare() const { return type_ == DECLARE; }
    bool is_literal() const { return type_ == LITERAL; }
    DEF_PTR_PROP_RW(LambdaLiteral, body);
    inline size_t parameters_size() const;
    inline FunctionPrototype::Parameter parameter(size_t i) const;
    inline TypeSign *return_type() const;
    inline FunctionPrototype *prototype() const;
    inline size_t statements_size() const;
    inline Statement *statement(size_t i) const;
    
    DEFINE_AST_NODE(FunctionDefinition);
private:
    Type type_;
    union {
        LambdaLiteral *body_;
        FunctionPrototype *prototype_;
    };
}; // class FunctionDefinition


class PartialInterfaceDefinition : public Definition {
public:
    DEF_ARENA_VECTOR_GETTER(FunctionDefinition *, method);
    
    virtual bool FieldExist(std::string_view name) = 0;
    
    bool InsertMethod(FunctionDefinition *method) {
        if (FieldExist(method->identifier()->ToSlice())) {
            return false;
        }
        methods_.push_back(method);
        named_methods_[method->identifier()->ToSlice()] = method;
        return true;
    }
    
    FunctionDefinition *FindMethodOrNull(std::string_view name) {
        auto iter = named_methods_.find(name);
        return iter == named_methods_.end() ? nullptr : iter->second;
    }
protected:
    PartialInterfaceDefinition(int position, Kind kind,
                               const ASTString *identifier,
                               base::ArenaVector<FunctionDefinition *> &&methods,
                               base::ArenaMap<std::string_view, FunctionDefinition *> &&named_methods)
        : Definition(position, kind, identifier)
        , methods_(std::move(methods))
        , named_methods_(named_methods) {}

    PartialInterfaceDefinition(base::Arena *arena, int position, Kind kind,
                               const ASTString *identifier)
        : Definition(position, kind, identifier)
        , methods_(arena)
        , named_methods_(arena) {
    }
    
    base::ArenaVector<FunctionDefinition *> methods_;
    base::ArenaMap<std::string_view, FunctionDefinition *> named_methods_;
}; // class PartialInterfaceDefinition


class StructureDefinition : public PartialInterfaceDefinition {
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

    DEF_ARENA_VECTOR_GETTER(Field, field);
    
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
protected:
    StructureDefinition(base::Arena *arena, int position, Kind kind, const ASTString *identifier,
                        base::ArenaVector<Field> &&fields)
        : PartialInterfaceDefinition(arena, position, kind, identifier)
        , fields_(std::move(fields))
        , named_fields_(arena) {}

    bool FieldExist(std::string_view name) override {
        return FindFieldOrNull(name) != nullptr;
    }

    base::ArenaVector<Field> fields_;
    base::ArenaMap<std::string_view, size_t> named_fields_;
}; // class StructureDefinition


class InterfaceDefinition : public PartialInterfaceDefinition {
public:
    InterfaceDefinition(int position, const ASTString *identifier,
                        base::ArenaVector<FunctionDefinition *> &&methods,
                        base::ArenaMap<std::string_view, FunctionDefinition *> &&named_methods)
        : PartialInterfaceDefinition(position, kInterfaceDefinition, identifier, std::move(methods),
                                     std::move(named_methods)) {}
    
    bool HasImplement(StructureDefinition *def) const {
        for (auto method : methods()) {
            if (auto impl = def->FindMethodOrNull(method->identifier()->ToSlice()); !impl) {
                return false;
            } else {
                if (!method->prototype()->IsAccept(impl->prototype())) {
                    return false;
                }
            }
        }
        return true;
    }
    
    DEFINE_AST_NODE(InterfaceDefinition);
private:
    bool FieldExist(std::string_view name) override { return FindMethodOrNull(name) != nullptr; }
}; // class InterfaceDefinition


class ObjectDefinition : public StructureDefinition {
public:
    ObjectDefinition(base::Arena *arena, int position, const ASTString *name,
                     base::ArenaVector<Field> &&fields)
        : StructureDefinition(arena, position, kObjectDefinition, name, std::move(fields)) {}
    
    DEFINE_AST_NODE(ObjectDefinition);
}; // class ObjectDefinition


class ClassDefinition : public StructureDefinition {
public:
    struct Parameter {
        bool field_declaration;
        union {
            int as_field; // index of field
            VariableDeclaration *as_parameter;
        };
    }; // struct Field

    ClassDefinition(base::Arena *arena,
                    int position,
                    const ASTString *name,
                    base::ArenaVector<Parameter> &&constructor,
                    base::ArenaVector<Field> &&fields,
                    const ASTString *base_prefix,
                    const ASTString *base_name,
                    base::ArenaVector<Expression *> &&arguments);

    DEF_ARENA_VECTOR_GETTER(Parameter, parameter);
    DEF_ARENA_VECTOR_GETTER(Expression *, argument);
    DEF_PTR_PROP_RW(const ASTString, base_prefix);
    DEF_PTR_PROP_RW(const ASTString, base_name);
    DEF_PTR_PROP_RW(ClassDefinition, base);
    
    bool SameOrBaseOf(ClassDefinition *base) const { return base == this || BaseOf(base); }

    bool BaseOf(ClassDefinition *base) const {
        for (ClassDefinition *i = base_; i != nullptr; i = i->base()) {
            if (i == base) {
                return true;
            }
        }
        return false;
    }
    
    int MakeParameterLookupTable();
    
    std::string ToBaseSymbolString() const {
        std::string buf(!base_prefix_ ? "" : base_prefix_->ToString());
        if (base_prefix_) {
            buf.append(1, '.');
        }
        return buf.append(DCHECK_NOTNULL(base_name_)->ToString());
    }
    
    std::tuple<ClassDefinition*, Field*> ResolveField(std::string_view name) {
        for (ClassDefinition *clazz = this; clazz != nullptr; clazz = clazz->base_) {
            if (auto found = clazz->FindFieldOrNull(name)) {
                return {clazz, found};
            }
        }
        return {nullptr, nullptr};
    }
    
    std::tuple<ClassDefinition*, FunctionDefinition *> ResolveMethod(std::string_view name) {
        for (ClassDefinition *clazz = this; clazz != nullptr; clazz = clazz->base_) {
            //printf("clazz: %s\n", clazz->identifier()->data());
            if (auto found = clazz->FindMethodOrNull(name)) {
                return {clazz, found};
            }
        }
        return {nullptr, nullptr};
    }

    DEFINE_AST_NODE(ClassDefinition);
private:
    base::ArenaVector<Parameter> parameters_;
    base::ArenaMap<std::string_view, size_t> named_parameters_;
    const ASTString *base_prefix_;
    const ASTString *base_name_;
    ClassDefinition *base_;
    base::ArenaVector<Expression *> arguments_;
}; // class ClassDefinition


class ClassImplementsBlock : public Statement {
public:
    ClassImplementsBlock(int position, const ASTString *prefix, const ASTString *name,
                         base::ArenaVector<FunctionDefinition *> &&methods)
        : Statement(position, kClassImplementsBlock)
        , prefix_(prefix)
        , name_(name)
        , owner_(nullptr)
        , methods_(methods) {}

    DEF_PTR_GETTER(const ASTString, prefix);
    DEF_PTR_GETTER(const ASTString, name);
    DEF_PTR_PROP_RW(StructureDefinition, owner);
    DEF_ARENA_VECTOR_GETTER(FunctionDefinition *, method);
    
    std::string ToSymbolString() const { return ToSymbolString(prefix_); }
    
    std::string ToSymbolString(const ASTString *prefix) const {
        std::string buf(!prefix ? "" : prefix->ToString());
        if (prefix) {
            buf.append(1, '.');
        }
        return buf.append(name_->ToString());
    }
    
    DEFINE_AST_NODE(ClassImplementsBlock);
private:
    const ASTString *prefix_;
    const ASTString *name_;
    StructureDefinition *owner_;
    base::ArenaVector<FunctionDefinition *> methods_;
}; // class ClassImplementsBlock


class CatchBlock : public StatementBlock {
public:
    CatchBlock(int position, VariableDeclaration *declaration,
               base::ArenaVector<Statement *> &&statements)
        : StatementBlock(position, kStatementBlock, std::move(statements))
        , expected_declaration_(DCHECK_NOTNULL(declaration)) {}
    
    DEF_PTR_PROP_RW(VariableDeclaration, expected_declaration);
private:
    VariableDeclaration *expected_declaration_;
}; // class CatchBlock


class TryCatchFinallyBlock : public Statement {
public:
    TryCatchFinallyBlock(int position, base::ArenaVector<Statement *> &&try_statements,
                         base::ArenaVector<CatchBlock *> &&catch_blocks,
                         base::ArenaVector<Statement *> &&finally_statements);
    
    DEF_ARENA_VECTOR_GETTER(Statement *, try_statement);
    DEF_ARENA_VECTOR_GETTER(Statement *, finally_statement);
    DEF_ARENA_VECTOR_GETTER(CatchBlock *, catch_block);
    
    DEFINE_AST_NODE(TryCatchFinallyBlock);
private:
    base::ArenaVector<Statement *> try_statements_;
    base::ArenaVector<CatchBlock *> catch_blocks_;
    base::ArenaVector<Statement *> finally_statements_;
}; // class TryCatchFinallyBlock


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
        THROW,
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

    DEF_VAL_GETTER(Control, control);
    DEF_PTR_PROP_RW(Expression, value);

    DEFINE_AST_NODE(BreakableStatement);
private:
    Control control_;
    Expression *value_;
}; // class BreakableStatement


class Literal : public Expression {
protected:
    Literal(int position, Kind kind): Expression(position, kind) {}
}; // class Literal


template<class T>
class LiteralBase : public Literal {
public:
    DEF_VAL_GETTER(T, value);
protected:
    LiteralBase(int position, Kind kind, T value)
        : Literal(position, kind)
        , value_(value) {}
    T value_;
}; // class LiteralBase

#define DEFINE_SIMPLE_LITERAL(name, value_type) \
    class name##Literal : public LiteralBase< value_type > { \
    public: \
        name##Literal(int position, value_type value) \
            : LiteralBase(position, k##name##Literal, value) {} \
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
    LambdaLiteral(int position, FunctionPrototype *proto, base::ArenaVector<Statement *> &&statements)
        : Literal(position, kLambdaLiteral)
        , prototype_(DCHECK_NOTNULL(proto))
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
    DEF_PTR_PROP_RW(ASTNode, scope);
    DEF_PTR_PROP_RW(Symbolize, original);
    
    bool IsFromGlobalScope() const { return scope_ && scope_->IsFileUnit(); }
    bool IsFromFunctionScope() const { return scope_ && scope_->IsFunctionDefinition(); }
    bool IsFromConstructorScope() const { return scope_ && scope_->IsClassDefinition(); }
    
    TypeSign *type() const {
        return !original_ ? nullptr : original_->AsVariableDeclaration()->type();
    }

    DEFINE_AST_NODE(Identifier);
private:
    const ASTString *name_;
    ASTNode *scope_;
    Symbolize *original_;
}; // class Identifier

class UnaryExpression : public Expression {
public:
    UnaryExpression(int position, Operator op, Expression *operand)
        : Expression(position, kUnaryExpression)
        , op_(op)
        , operand_(operand) {}

    DEF_VAL_GETTER(Operator, op);
    DEF_PTR_PROP_RW(Expression, operand);
    
    //Operator GetOperator() const override { return op(); }
    

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
    
    //Operator GetOperator() const override { return op(); }
    
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
        , element_type_(element_type)
        , reserve_(nullptr) {}
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


class IfExpression : public Expression {
public:
    IfExpression(int position, Statement *extra_statement, Expression *condition,
                 Statement *branch_true, Statement *branch_false)
        : Expression(position, kIfExpression)
        , extra_statement_(extra_statement)
        , condition_(DCHECK_NOTNULL(condition))
        , branch_true_(DCHECK_NOTNULL(branch_true))
        , branch_false_(branch_false) {
    }

    DEF_PTR_PROP_RW(Statement, extra_statement);
    DEF_PTR_PROP_RW(Expression, condition);
    DEF_PTR_PROP_RW(Statement, branch_true);
    DEF_PTR_PROP_RW(Statement, branch_false);
    
    DEFINE_AST_NODE(IfExpression);
private:
    Statement *extra_statement_;
    Expression *condition_;
    Statement *branch_true_;
    Statement *branch_false_;
}; // class IfExpression


class TypeExpression : public Expression {
public:
    DEF_PTR_PROP_RW(Expression, operand);
    DEF_PTR_PROP_RW(TypeSign, type);
protected:
    TypeExpression(int position, Kind kind, Expression *operand, TypeSign *type)
        : Expression(position, kind)
        , operand_(operand)
        , type_(type) {}
    
    Expression *operand_;
    TypeSign *type_;
}; // class TypeExpression


class TypeCastExpression : public TypeExpression {
public:
    TypeCastExpression(int position, Expression *operand, TypeSign *type)
        : TypeExpression(position, kTypeCastExpression, operand, type) {}

    DEFINE_AST_NODE(TypeCastExpression);
}; // class TypeCastExpression


class TypeTestExpression : public TypeExpression {
public:
    TypeTestExpression(int position, Expression *operand, TypeSign *type)
        : TypeExpression(position, kTypeTestExpression, operand, type) {}

    DEFINE_AST_NODE(TypeTestExpression);
}; // class TypeCastExpression


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
    return is_literal() ? body_->prototype() : prototype_;
}

inline size_t FunctionDefinition::statements_size() const {
    return is_literal() ? body_->statements_size() : 0;
}

inline Statement *FunctionDefinition::statement(size_t i) const {
    return is_literal() ? body_->statement(i) : nullptr;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_AST_H_
