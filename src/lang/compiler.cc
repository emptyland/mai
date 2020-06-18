#include "lang/compiler.h"
#include "lang/type-checker.h"
#include "lang/bytecode-generator.h"
#include "lang/isolate-inl.h"
#include "lang/parser.h"
#include "lang/ast.h"
#include "lang/lexer.h"
#include "lang/compilation-worker.h"
#include "lang/machine.h"
#include "lang/coroutine.h"
#include "lang/simplified-code-generator.h"
#include "base/arenas.h"

namespace mai {

namespace lang {

Error SourceFileResolve::Resolve(const std::string &dot_path, std::vector<FileUnit *> *file_units) {
    std::string dir(dot_path);
    for (size_t i = 0; i < dir.length(); i++) {
        if (dir[i] == '.') {
            dir[i] = '/';
        }
    }
    
    if (auto rs = env_->FileExists(dir); rs.fail()) {
        bool ok = false;
        for (auto search : search_path_) {
            if (rs = env_->FileExists(search + "/" + dir); rs.ok()) {
                dir = search + "/" + dir;
                ok = true;
                break;
            }
        }
        if (!ok) {
            return rs;
        }
    }

    std::vector<std::string> files;
    if (auto rs = Compiler::FindSourceFiles(dir, env_, false, &files); rs.fail()) {
        return rs;
    }
    return ParseAll(files, file_units);
}

Error SourceFileResolve::ParseAll(const std::vector<std::string> &source_files,
                                  std::vector<FileUnit *> *file_units) {
    int compile_fail = 0;
    Parser parser(arena_, feedback_);
    for (auto file_name : source_files) {
        std::unique_ptr<SequentialFile> input;
        if (auto rs = env_->NewSequentialFile(file_name, &input); rs.fail()) {
            return rs;
        }
        parser.SwitchInputFile(file_name, input.get());
        bool ok = true;
        FileUnit *unit = parser.Parse(&ok);
        if (ok) {
            file_units->push_back(unit);
        } else {
            compile_fail++;
        }
    }
    if (compile_fail > 0) {
        return MAI_CORRUPTION("Compile fail!");
    }
    return Error::OK();
}


void CompilationInfo::Print(base::AbstractPrinter *printer) const {
    printer->Println("---- [start] [%s] ----", start_fun_->name());
    
    std::stack<const Function *> frames;
    frames.push(invoke_info_.find(0)->second.fun);

    base::StandaloneArena arena;
    for (size_t i = 0; i < linear_path_.size(); i++) {
        BytecodeNode *node = BytecodeNode::From(&arena, linear_path_[i]);
        if (node->id() == kCheckStack) {
            const Function *fun = invoke_info_.find(i)->second.fun;
            frames.push(fun);
            printer->Println("---- [%zd] [%s] ----", frames.size(), fun->name());
        }
        printer->Printf("    [%03d] ", associated_pc_[i]);
        node->Print(printer);
        printer->Append("\n");
        if (node->id() == kReturn) {
            frames.pop();
            const Function *fun = frames.top();
            printer->Println("---- [%zd] [%s] ----", frames.size(), fun->name());
        }
    }
}


/*static*/ Error Compiler::FindSourceFiles(const std::string &dir, Env *env, bool unittest,
                                           std::vector<std::string> *files) {
    if (auto rs = env->FileExists(dir); rs.fail()) {
        return rs;
    }
    std::vector<std::string> children;
    if (auto rs = env->GetChildren(dir, &children); rs.fail()) {
        return rs;
    }

    for (auto child : children) {
        bool isdir = false;
        child = dir + "/" + child;
        if (auto rs = env->IsDirectory(child, &isdir); rs.fail()) {
            return rs;
        }
        if (isdir) {
            if (auto rs = FindSourceFiles(child, env, unittest, files); rs.fail()) {
                return rs;
            }
        } else {
            if (child.length() - child.rfind(kFileExtName) == sizeof(kFileExtName) - 1) {
                files->push_back(child);
            }
        }
    }
    return Error::OK();
}

/*static*/ Error Compiler::CompileInterpretion(Isolate *isolate,
                                               const std::string &dir,
                                               SyntaxFeedback *feedback,
                                               Function **init0,
                                               base::Arena *arena) {
    std::vector<std::string> files;
    if (auto rs = Compiler::FindSourceFiles(isolate->base_pkg_dir(), isolate->env(), false, &files);
        !rs) {
        return rs;
    }
    SourceFileResolve resolver(isolate->env(), arena, feedback, {}/*TODO*/);
    std::vector<FileUnit *> base_units;
    if (auto rs = resolver.ParseAll(files, &base_units); !rs) {
        return rs;
    }
    
    TypeChecker checker(arena, feedback);
    if (auto rs = checker.AddBootFileUnits(isolate->base_pkg_dir(), base_units, &resolver); !rs) {
        return rs;
    }

    files.clear();
    if (auto rs = Compiler::FindSourceFiles(dir, isolate->env(), false, &files); !rs) {
        return rs;
    }
    resolver.mutable_search_path()->insert(dir);
    
    std::vector<FileUnit *> units;
    if (auto rs = resolver.ParseAll(files, &units); !rs) {
        return rs;
    }
    if (auto rs = checker.AddBootFileUnits(dir, units, &resolver); !rs) {
        return rs;
    }
    if (!checker.Prepare() || !checker.Check()) {
        return MAI_CORRUPTION("Type check fail");
    }

    BytecodeGenerator generator(isolate, feedback, checker.class_exception(), checker.class_object(),
                                std::move(*checker.mutable_path_units()),
                                std::move(*checker.mutable_pkg_units()),
                                arena);
    if (!generator.Prepare() || !generator.Generate()) {
        return MAI_CORRUPTION("Code generated fail!");
    }

    *init0 = generator.generated_init0_fun();
    return Error::OK();
}

class SimplifiedCodeGenerationJob : public CompilationJob {
public:
    SimplifiedCodeGenerationJob(const CompilationInfo *compilation_info,
                                bool enable_debug,
                                bool enable_jit)
        : compilation_info_(compilation_info)
        , enable_debug_(enable_debug)
        , enable_jit_(enable_jit) {
    }
    
    virtual ~SimplifiedCodeGenerationJob() override {
        delete compilation_info_;
    }

    void Run() override {
        base::StandaloneArena arena;
        Kode *code = GenerateSimplifiedCode(compilation_info_, enable_debug_, enable_jit_, &arena);
        if (compilation_info_->mach()->running() == compilation_info_->owns()) {
            compilation_info_->owns()->set_hot_path(code);
        }
    }

    void Dispose() override {

    }

    DISALLOW_IMPLICIT_CONSTRUCTORS(SimplifiedCodeGenerationJob);
private:
    const CompilationInfo *const compilation_info_;
    const bool enable_debug_;
    const bool enable_jit_;
    Kode *code_;
}; // class SimplifiedCodeGenerationJob

/*static*/ Error Compiler::PostTracingBasedJob(Isolate *isolate,
                                               const CompilationInfo *compilation_info,
                                               int optimition_level,
                                               bool enable_debug,
                                               bool enable_jit) {
    switch (optimition_level) {
        case 1: {
            CompilationWorker *worker = DCHECK_NOTNULL(STATE->compilation_worker());
            worker->Commit(new SimplifiedCodeGenerationJob(compilation_info, enable_debug,
                                                           enable_jit));
        } break;

        default:
            return MAI_NOT_SUPPORTED("Not supported others level");
    }
    return Error::OK();
}

} // namespace lang

} // namespace mai
