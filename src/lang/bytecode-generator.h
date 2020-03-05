#pragma once
#ifndef MAI_LANG_BYTECODE_GENERATOR_H_
#define MAI_LANG_BYTECODE_GENERATOR_H_

#include "lang/stable-space-builder.h"
#include "lang/ast.h"
#include "lang/syntax.h"
#include <map>
#include <set>
#include <unordered_set>

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
        };
        Linkage linkage;
        Class *type;
        int index;
    }; // struct BytecodeGenerator::Value

    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback,
                      ClassDefinition *class_exception,
                      ClassDefinition *class_any,
                      std::map<std::string, std::vector<FileUnit *>> &&path_units,
                      std::map<std::string, std::vector<FileUnit *>> &&pkg_units);
    
    BytecodeGenerator(Isolate *isolate, SyntaxFeedback *feedback);

    bool Generate();
private:
    class AbstractScope;
    class FileScope;
    class FunctionScope;
    class BlockScope;
    
    Function *BuildFunction(const std::string &name, FunctionScope *scope);
    
    Result VisitTypeSign(TypeSign *) override;
    Result VisitClassDefinition(ClassDefinition *) override;
    Result VisitObjectDefinition(ObjectDefinition *) override;
    Result VisitFunctionDefinition(FunctionDefinition *) override;
    Result VisitVariableDeclaration(VariableDeclaration *) override;
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
    
    void LdaIfNeeded(const Class *clazz, int index, int linkage, BytecodeArrayBuilder *emitter);
    void LdaStack(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    void LdaConst(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    void LdaGlobal(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    void LdaCaptured(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    void StaIfNeeded(const Class *clazz, int index, int linkage, BytecodeArrayBuilder *emitter);
    void StaStack(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    void StaConst(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    void StaGlobal(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    void StaCaptured(const Class *clazz, int index, BytecodeArrayBuilder *emitter);
    
    bool PrepareUnit(const std::string &pkg_name, FileUnit *unit);
    int LinkGlobalVariable(VariableDeclaration *var);
    
    bool HasGenerated(ASTNode *ast) const {
        return symbol_trace_.find(ast) != symbol_trace_.end();
    }
    
    Isolate *isolate_;
    MetadataSpace *metadata_space_;
    SyntaxFeedback *error_feedback_;
    ClassDefinition *class_exception_;
    ClassDefinition *class_any_;
    std::map<std::string, std::vector<FileUnit *>> path_units_;
    std::map<std::string, std::vector<FileUnit *>> pkg_units_;
    std::map<std::string, Value> symbols_;
    std::unordered_set<void *> symbol_trace_;
    GlobalSpaceBuilder global_space_;
    AbstractScope *current_ = nullptr;
    FunctionScope *current_fun_ = nullptr;
    FileScope     *current_file_ = nullptr;
    Function *generated_init0_fun_ = nullptr;
    Function *generated_main_fun_ = nullptr;
}; // class BytecodeGenerator

} // namespace lang

} // namespace mai

#endif // MAI_LANG_BYTECODE_GENERATOR_H_
