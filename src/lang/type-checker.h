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
    Result VisitVariableDeclaration(VariableDeclaration *) override;
    Result VisitDotExpression(DotExpression *) override;
    
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
    bool CheckFileUnit(const std::string &pkg_name, FileUnit *unit);
    
    Result CheckDotExpression(TypeSign *type, DotExpression *ast);
    
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
    
    virtual Symbolize *ResolveOrNull(const ASTString *name) { return nullptr; }

    AbstractScope *GetScope(Kind kind) {
        for (AbstractScope *scope = this; scope != nullptr; scope = scope->prev_) {
            if (scope->kind_ == kind) {
                return scope;
            }
        }
        return nullptr;
    }
    
    FileScope *GetFileScope() { return reinterpret_cast<FileScope *>(GetScope(kFileScope)); }
    
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
    
    Symbolize *ResolveOrNull(const ASTString *name) override;
    Symbolize *ResolveOrNull(const ASTString *prefix, const ASTString *name);
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(FileScope);
private:
    FileUnit *file_unit_;
    const std::map<std::string, Symbolize *> *all_symbols_;
    std::set<std::string> all_name_space_;
    std::map<std::string, std::string> alias_name_space_;
}; // class FileScope

class ClassScope {
public:
    
}; // class ClassScope

class FunctionScope {
public:
    
}; // class FunctionScope

class BlockScope {
public:
    
}; // class BlockScope


//inline FileScope *AbstractScope::GetFileScope() {
//    if (kind() == kFileScope) {
//        return static_cast<FileScope *>(this);
//    }
//
//    AbstractScope *scope = prev_;
//    while (scope) {
//        if (scope->kind() == kFileScope) {
//            reutrn
//        }
//        scope = scope->prev_;
//    }
//}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_TYPE_CHECKER_H_
