#pragma once
#ifndef MAI_LANG_COMPILER_H_
#define MAI_LANG_COMPILER_H_

#include "lang/syntax.h"
#include "lang/bytecode.h"
#include "base/base.h"
#include "mai/error.h"
#include <map>
#include <set>

namespace mai {
namespace base {
class AbstractPrinter;
} // namespace base
class Env;
namespace lang {

class FileUnit;
class Isolate;
class Function;
class Machine;
class Coroutine;

class SourceFileResolve {
public:
    SourceFileResolve(Env *env, base::Arena *arena, SyntaxFeedback *feedback,
                      const std::set<std::string> &search_path)
        : env_(env)
        , arena_(arena)
        , feedback_(feedback)
        , search_path_(search_path) {}

    Error Resolve(const std::string &dot_path, std::vector<FileUnit *> *file_units);
    Error ParseAll(const std::vector<std::string> &source_files, std::vector<FileUnit *> *file_units);
    
    DEF_VAL_PROP_RMW(std::set<std::string>, search_path);
    DISALLOW_IMPLICIT_CONSTRUCTORS(SourceFileResolve);
private:
    Env *env_;
    base::Arena *arena_;
    SyntaxFeedback *feedback_;
    std::set<std::string> search_path_;
}; // class SourceFileResolve

class CompilationInfo {
public:
    struct InvokeInfo {
        Function *fun;
        int slot;
        size_t position;
    };
    
    CompilationInfo(Machine *mach,
                    Coroutine *owns,
                    const Function *start_fun,
                    int start_slot,
                    int start_pc,
                    std::vector<BytecodeInstruction> &&linear_path,
                    std::vector<uint32_t> &&associated_pc,
                    std::map<size_t, InvokeInfo> &&invoke_info)
        : mach_(mach)
        , owns_(owns)
        , start_fun_(start_fun)
        , start_slot_(start_slot)
        , start_pc_(start_pc)
        , linear_path_(linear_path)
        , associated_pc_(associated_pc)
        , invoke_info_(invoke_info) {}

    DEF_PTR_GETTER(Machine, mach);
    DEF_PTR_GETTER(Coroutine, owns);
    DEF_VAL_GETTER(int, start_slot);
    DEF_VAL_GETTER(int, start_pc);
    DEF_PTR_GETTER(const Function, start_fun);
    DEF_VAL_GETTER(std::vector<BytecodeInstruction>, linear_path);
    DEF_VAL_GETTER(std::vector<uint32_t>, associated_pc);
    const std::map<size_t, InvokeInfo> &invoke_info() const { return invoke_info_; }

    void Print(base::AbstractPrinter *printer) const;
private:
    // Associated machine(thread)
    Machine *const mach_;
    Coroutine *const owns_;
    // Start function
    const Function *const start_fun_;
    // Start slot
    const int start_slot_;
    // Start pc
    const int start_pc_;
    // Traced linear byte-code instruction path
    const std::vector<BytecodeInstruction> linear_path_;
    // PCs of associated to linear_path_
    const std::vector<uint32_t> associated_pc_;
    // Invoke functions info
    const std::map<size_t, InvokeInfo> invoke_info_;
};

struct Compiler {
    
    static constexpr const char kFileExtName[] = ".mai";
    static constexpr const char kTestFilePostfix[] = "-test.mai";

    static Error FindSourceFiles(const std::string &dir, Env *env, bool unittest,
                                 std::vector<std::string> *files);
    
    static Error CompileInterpretion(Isolate *isolate,
                                     const std::string &dir,
                                     SyntaxFeedback *feedback,
                                     Function **init0,
                                     base::Arena *arena);
    
    static Error PostTracingBasedJob(Isolate *isolate,
                                     const CompilationInfo *compilation_info,
                                     int optimition_level,
                                     bool enable_debug,
                                     bool enable_jit);
    
}; // struct Compiler

} // namespace lang

} // namespace mai

#endif // MAI_LANG_COMPILER_H_
