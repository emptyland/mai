#pragma once
#ifndef MAI_LANG_TYPE_CHECKER_H_
#define MAI_LANG_TYPE_CHECKER_H_

#include "lang/ast.h"
#include "lang/syntax.h"
#include <map>
#include <set>
#include <unordered_set>

namespace mai {

namespace lang {

class SourceFileResolve;

class TypeChecker final : public PartialASTVisitor {
public:
    TypeChecker(base::Arena *arena, SyntaxFeedback *feedback);
    ~TypeChecker() override {}

    DEF_PTR_GETTER(ClassDefinition, class_object);
    DEF_PTR_GETTER(ClassDefinition, class_exception);
    std::map<std::string, std::vector<FileUnit *>> *mutable_path_units() { return &path_units_; };
    std::map<std::string, std::vector<FileUnit *>> *mutable_pkg_units() { return &pkg_units_; }
    
    Error AddBootFileUnits(const std::string &original_path,
                           const std::vector<FileUnit *> &file_units, SourceFileResolve *resolve);
    bool Prepare();
    bool Check();
    
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
    class Scope;
    class FileScope;
    class ClassScope;
    class FunctionScope;
    class BlockScope;

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
    Result VisitTypeCastExpression(TypeCastExpression *) override;
    Result VisitTypeTestExpression(TypeTestExpression *) override;
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
    Result VisitWhileLoop(WhileLoop *) override;
    Result VisitForLoop(ForLoop *) override;
    Result VisitIfExpression(IfExpression *) override;
    Result VisitStatementBlock(StatementBlock *) override;
    Result VisitTryCatchFinallyBlock(TryCatchFinallyBlock *) override;
    Result VisitRunStatement(RunStatement *) override;

    bool PrepareClassDefinition(FileUnit *unit);
    bool CheckFileUnit(const std::string &pkg_name, FileUnit *unit);
    
    Result CheckForStep(ForLoop *ast);
    Result CheckForIterate(ForLoop *ast);
    Result CheckForChannel(ForLoop *ast);
    Result CheckDotExpression(TypeSign *type, DotExpression *ast);
    Result CheckClassOrObjectFieldAccess(TypeSign *type, DotExpression *ast);
    Result CheckObjectFieldAccess(ObjectDefinition *object, DotExpression *ast);
    bool CheckModifitionAccess(ASTNode *ast);
    bool CheckAddExpression(ASTNode *ast, TypeSign *lhs, TypeSign *rhs);
    bool CheckSubExpression(ASTNode *ast, TypeSign *lhs, TypeSign *rhs);
    bool CheckSymbolDependence(Symbolize *sym);
    bool CheckDuplicatedSymbol(const std::string &pkg_name, const Symbolize *sym,
                               FileScope *file_scope);
    
    inline SourceLocation FindSourceLocation(const ASTNode *ast);
    
    bool HasSymbolChecked(Symbolize *sym) const {
        return symbol_trace_.find(sym) != symbol_trace_.end();
    }
    
    base::Arena *arena_;
    SyntaxFeedback *error_feedback_;
    TypeSign *const kVoid;
    TypeSign *const kError;
    ClassDefinition *class_exception_ = nullptr;
    ClassDefinition *class_object_ = nullptr;
    Scope *current_ = nullptr;
    std::map<std::string, std::vector<FileUnit *>> path_units_;
    std::map<std::string, std::vector<FileUnit *>> pkg_units_;
    std::map<std::string, Symbolize *> symbols_;
    std::unordered_set<void *> symbol_trace_;
    std::unordered_set<void *> symbol_track_;
}; // class TypeChecker

} // namespace lang

} // namespace mai

#endif // MAI_LANG_TYPE_CHECKER_H_
