#pragma once
#ifndef MAI_LANG_TYPE_CHECKER_H_
#define MAI_LANG_TYPE_CHECKER_H_

#include "lang/ast.h"
#include "lang/syntax.h"
#include <map>
#include <set>

namespace mai {

namespace lang {

class SourceFileResolve;
class AbstractScope;
class FileScope;
class ClassScope;
class FunctionScope;
class BlockScope;

class TypeChecker : public PartialASTVisitor {
public:
    TypeChecker(base::Arena *arena, SyntaxFeedback *feedback);
    ~TypeChecker() override {}
    
    Error AddBootFileUnits(const std::string &original_path,
                           const std::vector<FileUnit *> &file_units, SourceFileResolve *resolve);
    bool Prepare();
    bool Check();
    
    Result VisitTypeSign(TypeSign *) override;
    Result VisitBoolLiteral(BoolLiteral *) override;
    Result VisitI8Literal(I8Literal *) override;
    Result VisitU8Literal(U8Literal *) override;
    Result VisitI16Literal(I16Literal *) override;
    Result VisitU16Literal(U16Literal *) override;
    Result VisitI32Literal(I32Literal *) override;
    Result VisitU32Literal(U32Literal *) override;
    Result VisitIntLiteral(IntLiteral *) override;
    Result VisitUIntLiteral(UIntLiteral *) override;
    Result VisitI64Literal(I64Literal *) override;
    Result VisitU64Literal(U64Literal *) override;
    Result VisitF32Literal(F32Literal *) override;
    Result VisitF64Literal(F64Literal *) override;
    Result VisitStringLiteral(StringLiteral *) override;
    Result VisitNilLiteral(NilLiteral *) override;
    Result VisitLambdaLiteral(LambdaLiteral *) override;
    Result VisitCallExpression(CallExpression *) override;
    Result VisitPairExpression(PairExpression *) override;
    Result VisitIndexExpression(IndexExpression *) override;
    Result VisitUnaryExpression(UnaryExpression *) override;
    Result VisitArrayInitializer(ArrayInitializer *) override;
    Result VisitMapInitializer(MapInitializer *) override;
    Result VisitBinaryExpression(BinaryExpression *) override;
    Result VisitBreakableStatement(BreakableStatement *) override;
    Result VisitAssignmentStatement(AssignmentStatement *) override;
    Result VisitStringTemplateExpression(StringTemplateExpression *) override;
    Result VisitVariableDeclaration(VariableDeclaration *) override;
    Result VisitIdentifier(Identifier *) override;
    Result VisitDotExpression(DotExpression *) override;
    Result VisitFunctionDefinition(FunctionDefinition *) override;
    Result VisitInterfaceDefinition(InterfaceDefinition *) override;
    Result VisitObjectDefinition(ObjectDefinition *) override;
    Result VisitClassDefinition(ClassDefinition *) override;
    Result VisitClassImplementsBlock(ClassImplementsBlock *) override;
    
    std::vector<FileUnit *> FindPathUnits(const std::string &path) const {
        auto iter = path_units_.find(path);
        return iter == path_units_.end() ? std::vector<FileUnit *>{} : iter->second;
    }
    
    std::vector<FileUnit *> FindPackageUnits(const std::string &pkg) const {
        auto iter = pkg_units_.find(pkg);
        return iter == pkg_units_.end() ? std::vector<FileUnit *>{} : iter->second;
    }
    
    Symbolize *FindSymbolOrNull(const std::string &name) const {
        auto iter = symbols_.find(name);
        return iter == symbols_.end() ? nullptr : iter->second;
    }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(TypeChecker);
private:
    bool PrepareClassDefinition(FileUnit *unit);
    bool CheckFileUnit(const std::string &pkg_name, FileUnit *unit);
    
    Result CheckDotExpression(TypeSign *type, DotExpression *ast);
    Result CheckClassOrObjectFieldAccess(TypeSign *type, DotExpression *ast);
    Result CheckInDecrementAssignment(ASTNode *ast, TypeSign *lval, TypeSign *rval, const char *op);
    
    inline SourceLocation FindSourceLocation(ASTNode *ast);
    
    base::Arena *arena_;
    SyntaxFeedback *error_feedback_;
    TypeSign *const kVoid;
    TypeSign *const kError;
    
    AbstractScope *current_ = nullptr;
    std::map<std::string, std::vector<FileUnit *>> path_units_;
    std::map<std::string, std::vector<FileUnit *>> pkg_units_;
    base::ArenaVector<FileUnit *> all_units_;
    std::map<std::string, Symbolize *> symbols_;
    std::map<std::string, Definition *> classes_objects_;
}; // class TypeChecker

class AbstractScope {
public:
    enum Kind {
        kFileScope,
        kClassScope,
        kFunctionScope,
        kBlockScope,
    };
    ~AbstractScope() {
        DCHECK_EQ(this, *current_);
        *current_ = prev_;
    }
    
    DEF_VAL_GETTER(Kind, kind);
    DEF_PTR_GETTER(AbstractScope, prev);
    
    bool is_file_scope() const { return kind_ == kFileScope; }
    bool is_class_scope() const { return kind_ == kClassScope; }
    bool is_function_scope() const { return kind_ == kFunctionScope; }
    bool is_block_scope() const { return kind_ == kBlockScope; }
    
    virtual Symbolize *FindOrNull(const ASTString *name) { return nullptr; }
    virtual Symbolize *Register(Symbolize *sym) { return nullptr; }
    
    std::tuple<AbstractScope *, Symbolize *> Resolve(const ASTString *name) {
        for (auto i = this; i != nullptr; i = i->prev_) {
            if (Symbolize *sym = i->FindOrNull(name); sym != nullptr) {
                return {i, sym};
            }
        }
        return {nullptr, nullptr};
    }

    AbstractScope *GetScope(Kind kind) {
        for (AbstractScope *scope = this; scope != nullptr; scope = scope->prev_) {
            if (scope->kind_ == kind) {
                return scope;
            }
        }
        return nullptr;
    }
    
    inline FileScope *GetFileScope();
    inline ClassScope *GetClassScope();
    inline FunctionScope *GetFunctionScope();
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(AbstractScope);
protected:
    AbstractScope(Kind kind, AbstractScope **current)
        : kind_(kind)
        , prev_(*current)
        , current_(current) {
        *current_ = this;
    }
    
    Kind kind_;
    AbstractScope *prev_;
    AbstractScope **current_;
}; // class AbstractScope


class FileScope : public AbstractScope {
public:
    FileScope(const std::map<std::string, Symbolize *> *symbols, FileUnit *file_unit, AbstractScope **current)
        : AbstractScope(kFileScope, current)
        , file_unit_(file_unit)
        , all_symbols_(symbols) {
    }
    
    void Initialize(const std::map<std::string, std::vector<FileUnit *>> &path_units);
    
    DEF_PTR_GETTER(FileUnit, file_unit);
    
    Symbolize *FindOrNull(const ASTString *name) override;
    Symbolize *FindOrNull(const ASTString *prefix, const ASTString *name);

    DISALLOW_IMPLICIT_CONSTRUCTORS(FileScope);
private:
    FileUnit *file_unit_;
    const std::map<std::string, Symbolize *> *all_symbols_;
    std::set<std::string> all_name_space_;
    std::map<std::string, std::string> alias_name_space_;
}; // class FileScope

class ClassScope : public AbstractScope {
public:
    ClassScope(StructureDefinition *clazz, AbstractScope **current)
        : AbstractScope(kClassScope, current) , clazz_(clazz) {}
    
    ClassDefinition *clazz() { return DCHECK_NOTNULL(clazz_->AsClassDefinition()); }
    ObjectDefinition *object() { return DCHECK_NOTNULL(clazz_->AsObjectDefinition()); }

    DISALLOW_IMPLICIT_CONSTRUCTORS(ClassScope);
private:
    StructureDefinition *clazz_;
}; // class ClassScope

class BlockScope : public AbstractScope {
public:
    enum BlockKind {
        kIfBlock,
        kForBlock,
        kWhileBlock,
        kNormalBlock,
        kFunctionBlock,
    };
    
    BlockScope(BlockKind block_kind, Statement *stmt, AbstractScope **current)
        : BlockScope(kBlockScope, block_kind, stmt, current) {}
    
    Symbolize *FindOrNull(const ASTString *name) override {
        auto iter = locals_.find(name->ToSlice());
        return iter == locals_.end() ? nullptr : iter->second;
    }

    Symbolize *Register(Symbolize *sym) override {
        if (auto iter = locals_.find(sym->identifier()->ToSlice()); iter != locals_.end()) {
            return iter->second;
        }
        locals_[sym->identifier()->ToSlice()] = sym;
        return nullptr;
    }
protected:
    BlockScope(Kind kind, BlockKind block_kind, Statement *stmt, AbstractScope **current)
        : AbstractScope(kind, current)
        , block_kind_(block_kind)
        , stmt_(stmt) {
    }

    BlockKind block_kind_;
    Statement *stmt_;
    std::map<std::string_view, Symbolize *> locals_;
}; // class BlockScope


class FunctionScope : public BlockScope {
public:
    FunctionScope(LambdaLiteral *function, AbstractScope **current)
        : BlockScope(kFunctionScope, kFunctionBlock, function, current)
        , function_(function)
        , constructor_(nullptr) {
    }
    
    FunctionScope(ClassDefinition *constructor, AbstractScope **current);
    
    DEF_PTR_GETTER(LambdaLiteral, function);
    DEF_PTR_GETTER(ClassDefinition, constructor);
private:
    LambdaLiteral *function_;
    ClassDefinition *constructor_;
}; // class FunctionScope


inline SourceLocation TypeChecker::FindSourceLocation(ASTNode *ast) {
    return current_->GetFileScope()->file_unit()->FindSourceLocation(ast);
}

inline FileScope *AbstractScope::GetFileScope() {
    return down_cast<FileScope>(GetScope(kFileScope));
}

inline ClassScope *AbstractScope::GetClassScope() {
    return down_cast<ClassScope>(GetScope(kClassScope));
}

inline FunctionScope *AbstractScope::GetFunctionScope() {
    return down_cast<FunctionScope>(GetScope(kFunctionScope));
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_TYPE_CHECKER_H_
