#pragma once
#ifndef MAI_LANG_BYTECODE_GENERATOR_H_
#define MAI_LANG_BYTECODE_GENERATOR_H_

#include "lang/stable-space-builder.h"
#include "lang/ast.h"
#include "lang/syntax.h"
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

namespace mai {

namespace lang {

class Isolate;
class BytecodeArrayBuilder;

class BytecodeGenerator final : public PartialASTVisitor {
public:
    struct Value {
        enum Linkage {
            kError = 0, // Error flag
            kMetadata,
            kGlobal,
            kStack,
            kConstant,
            kCaptured,
            kACC,
        }; // enum Linkage
        Linkage linkage = kError;
        const Class *type = nullptr;
        int index = 0;
        Symbolize *ast = nullptr;
        uint32_t flags = 0;
    }; // struct BytecodeGenerator::Value

    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback,
                      ClassDefinition *class_exception,
                      ClassDefinition *class_object,
                      std::map<std::string, std::vector<FileUnit *>> &&path_units,
                      std::map<std::string, std::vector<FileUnit *>> &&pkg_units,
                      base::Arena *arena);
    
    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback, base::Arena *arena);
    
    ~BytecodeGenerator();
    
    bool Prepare();
    bool Generate();
    
    DEF_PTR_GETTER(Function, generated_init0_fun);
    
    Value FindValue(const std::string &name) const {
        if (auto iter = symbols_.find(name); iter != symbols_.end()) {
            return iter->second;
        } else {
            return {Value::kError};
        }
    }
    
    template<class T>
    inline T *FindGlobalOrNull(const std::string &name) {
        Value value = FindValue(name);
        if (value.linkage == Value::kError) {
            return nullptr;
        }
        DCHECK_EQ(Value::kGlobal, value.linkage);
        return reinterpret_cast<T *>(global_space_.address(value.index));
    }
private:
    class Scope;
    class FileScope;
    class ClassScope;
    class FunctionScope;
    class BlockScope;
    
    struct FieldDesc {
        const ASTString *name;
        const Class *type;
        uint32_t offset;
    };
    
    struct CallingReceiver {
        enum Kind {
            kVtab,
            kCtor,
            kNative,
            kBytecode,
        };
        Kind kind;
        uint32_t attributes;
        ASTNode *ast;
        int arguments_size;
    }; // struct CallingReceiver
    
    bool PrepareUnit(const std::string &pkg_name, FileUnit *unit);
    bool GenerateUnit(const std::string &pkg_name, FileUnit *unit);

    Function *BuildFunction(const std::string &name, FunctionScope *scope);
    
    Result VisitTypeSign(TypeSign *) override;
    Result VisitClassDefinition(ClassDefinition *) override;
    Result VisitObjectDefinition(ObjectDefinition *) override;
    Result VisitFunctionDefinition(FunctionDefinition *) override;
    Result VisitLambdaLiteral(LambdaLiteral *) override;
    Result VisitVariableDeclaration(VariableDeclaration *) override;
    Result VisitCallExpression(CallExpression *) override;
    Result VisitDotExpression(DotExpression *) override;
    Result VisitStringTemplateExpression(StringTemplateExpression *) override;
    Result VisitUnaryExpression(UnaryExpression *) override;
    Result VisitBinaryExpression(BinaryExpression *) override;
    Result VisitIfExpression(IfExpression *) override;
    Result VisitStatementBlock(StatementBlock *) override;
    Result VisitIdentifier(Identifier *) override;
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
    Result VisitNilLiteral(NilLiteral *) override;
    Result VisitStringLiteral(StringLiteral *) override;
    Result VisitBreakableStatement(BreakableStatement *) override;
    Result VisitTryCatchFinallyBlock(TryCatchFinallyBlock *) override;
    Result VisitAssignmentStatement(AssignmentStatement *) override;
    Result VisitRunStatement(RunStatement *) override;
    Result VisitWhileLoop(WhileLoop *) override;
    Result VisitForLoop(ForLoop *) override;
    
    Result GenerateForStep(ForLoop *ast);
    Result GenerateCalling(CallExpression *ast, CallingReceiver *receiver);
    uint32_t ProcessStructure(StructureDefinition *ast, uint32_t offset, ClassBuilder *builder,
                              std::vector<FieldDesc> *fields_desc);

    bool ShouldCaptureVar(Scope *owns, Value value);
    Result CaptureVar(const std::string &name, Scope *owns, Value value);
    SourceLocation FindSourceLocation(const ASTNode *ast);
    
    Function *GenerateLambdaLiteral(const std::string &name, bool is_method, LambdaLiteral *ast);
    Function *GenerateClassConstructor(const std::vector<FieldDesc> &fields_desc, const Class *base,
                                       ClassDefinition *ast);
    Function *GenerateObjectConstructor(const std::vector<FieldDesc> &fields_desc,
                                        ObjectDefinition *ast);
    bool ProcessStructureInitializer(const std::vector<FieldDesc> &fields_desc, int self_index,
                                     StructureDefinition *ast);
    bool ProcessMethodsPost(Function *ctor, StructureDefinition *ast);
    Result GenerateRegularCalling(CallExpression *ast, CallingReceiver *receiver);
    Result GenerateMethodCalling(Value primary, CallExpression *ast, CallingReceiver *receiver);
    Result GenerateNewObject(const Class *clazz, CallExpression *ast, CallingReceiver *receiver);
    int GenerateArguments(const base::ArenaVector<Expression *> &args,
                          const std::vector<const Class *> &params, ASTNode *self_ast, int self_idx,
                          bool vargs);
    Result GeneratePropertyAssignment(const ASTString *name, Value self, Scope *owns, Operator op,
                                      Expression *rhs, DotExpression *ast);
    Result GenerateVariableAssignment(const ASTString *name, Value lval, Scope *owns, Operator op,
                                      Expression *rhs, ASTNode *ast);
    Result GenerateIncrement(const Class *lval_type, int lval_index, Value::Linkage lval_linkage,
                             Operator op);
    Result GenerateStoreProperty(const Class *clazz, int index, Value::Linkage linkage,
                                 DotExpression *ast);
    Result GenerateLoadProperty(const Class *clazz, int index, Value::Linkage linkage,
                                DotExpression *ast);
    
    void GenerateComparation(const Class *clazz, Operator op, int lhs, int rhs, ASTNode *ast);
    void GenerateOperation(const Class *clazz, Operator op, int lhs_index,
                           Value::Linkage lhs_linkage, int rhs_index, Value::Linkage rhs_linkage,
                           ASTNode *ast);
    void GenerateOperation(const Class *clazz, Operator op, int lhs, int rhs, ASTNode *ast);
    void ToStringIfNeeded(const Class *clazz, int index, Value::Linkage linkage, ASTNode *ast);
    void InboxIfNeeded(const Class *clazz, int index, Value::Linkage linkage, const Class *lval,
                       ASTNode *ast);
    void MoveToStackIfNeeded(const Class *clazz, int index, Value::Linkage linkage, int dest,
                             ASTNode *ast);
    void MoveToArgumentIfNeeded(const Class *clazz, int index, Value::Linkage linkage, int dest,
                                ASTNode *ast);
    void LdaIfNeeded(const Result &rv, ASTNode *ast);
    void LdaIfNeeded(const Class *clazz, int index, Value::Linkage linkage, ASTNode *ast);
    void LdaStack(const Class *clazz, int index, ASTNode *ast);
    void LdaConst(const Class *clazz, int index, ASTNode *ast);
    void LdaGlobal(const Class *clazz, int index, ASTNode *ast);
    void LdaCaptured(const Class *clazz, int index, ASTNode *ast);
    void LdaProperty(const Class *clazz, int index, int offset, ASTNode *ast);
    void StaIfNeeded(const Class *clazz, int index, Value::Linkage linkage, ASTNode *ast);
    void StaStack(const Class *clazz, int index, ASTNode *ast);
    void StaConst(const Class *clazz, int index, ASTNode *ast);
    void StaGlobal(const Class *clazz, int index, ASTNode *ast);
    void StaCaptured(const Class *clazz, int index, ASTNode *ast);
    void StaProperty(const Class *clazz, int index, int offset, ASTNode *ast);
    
    bool GenerateSymbolDependence(Value value);
    bool GenerateSymbolDependence(Symbolize *ast);
    
    int LinkGlobalVariable(VariableDeclaration *var);
    Value FindOrInsertExternalFunction(const std::string &name);
    
    Value EnsureFindValue(const std::string &name) const {
        Value value = FindValue(name);
        DCHECK_NE(value.linkage, Value::kError);
        return value;
    }

    bool HasGenerated(ASTNode *ast) const { return symbol_trace_.find(ast) != symbol_trace_.end(); }
    
    bool IsMinorInitializer(const StructureDefinition *owns, const FunctionDefinition *fun) const;

    Isolate *isolate_;
    MetadataSpace *metadata_space_;
    SyntaxFeedback *error_feedback_;
    ClassDefinition *class_exception_;
    ClassDefinition *class_object_;
    std::map<std::string, std::vector<FileUnit *>> path_units_;
    std::map<std::string, std::vector<FileUnit *>> pkg_units_;
    std::unordered_map<std::string, Value> symbols_;
    std::unordered_set<void *> symbol_trace_;
    base::Arena *arena_;
    GlobalSpaceBuilder global_space_;
    FunctionScope *current_init0_fun_ = nullptr;
    FunctionScope *current_fun_ = nullptr;
    ClassScope    *current_class_ = nullptr;
    FileScope     *current_file_ = nullptr;
    Scope         *current_ = nullptr;
    Function *generated_init0_fun_ = nullptr;
}; // class BytecodeGenerator

} // namespace lang

} // namespace mai

#endif // MAI_LANG_BYTECODE_GENERATOR_H_
