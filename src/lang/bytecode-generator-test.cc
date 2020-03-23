#include "lang/bytecode-generator.h"
#include "lang/compiler.h"
#include "lang/type-checker.h"
#include "lang/lexer.h"
#include "lang/ast.h"
#include "lang/syntax-mock-test.h"
#include "test/isolate-initializer.h"
#include "base/arenas.h"
#include "base/slice.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class BytecodeGeneratorTest : public test::IsolateInitializer {
public:
    BytecodeGeneratorTest()
        : arena_(Env::Default()->GetLowLevelAllocator())
        , env_(Env::Default())
        , resolver_(env_, &arena_, &feedback_, {})
        , checker_(&arena_, &feedback_) {}
    
    void SetUp() override {
        IsolateInitializer::SetUp();
    }
    
    void TearDown() override {
        IsolateInitializer::TearDown();
        delete generator_;
        generator_ = nullptr;
    }
    
    Error Parse() { return Parse(suit_path_); }
    
    Error Parse(const std::string &dir);
    
    void AssertFunction(const std::string &name, const Function *fun);
    
    void Define(const std::string &suit_name) {
        ASSERT_TRUE(suit_path_.empty());
        suit_path_ = "tests/lang/" + suit_name;
    }

    std::string suit_path_;
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Env *env_;
    SourceFileResolve resolver_;
    TypeChecker checker_;
    BytecodeGenerator *generator_ = nullptr;
}; // class BytecodeGeneratorTest


Error BytecodeGeneratorTest::Parse(const std::string &dir) {
    std::vector<std::string> files;
    if (auto rs = Compiler::FindSourceFiles("src/lang/pkg", env_, false, &files); !rs) {
        return rs;
    }
    std::vector<FileUnit *> base_units;
    if (auto rs = resolver_.ParseAll(files, &base_units); !rs) {
        return rs;
    }
    if (auto rs = checker_.AddBootFileUnits("src/lang/pkg", base_units, &resolver_); !rs){
        return rs;
    }

    files.clear();
    if (auto rs = Compiler::FindSourceFiles(dir, env_, false, &files); !rs) {
        return rs;
    }
    resolver_.mutable_search_path()->insert(dir);
    
    std::vector<FileUnit *> units;
    if (auto rs = resolver_.ParseAll(files, &units); !rs) {
        return rs;
    }
    if (auto rs = checker_.AddBootFileUnits(dir, units, &resolver_); !rs) {
        return rs;
    }
    if (!checker_.Prepare() || !checker_.Check()) {
        return MAI_CORRUPTION("Check fail!");
    }
    
    generator_ = new BytecodeGenerator(isolate_, &feedback_,
                                       checker_.class_exception(),
                                       checker_.class_object(),
                                       std::move(*checker_.mutable_path_units()),
                                       std::move(*checker_.mutable_pkg_units()),
                                       &arena_);
    return Error::OK();
}

void BytecodeGeneratorTest::AssertFunction(const std::string &name, const Function *fun) {
    ASSERT_FALSE(suit_path_.empty());
    base::StringBuildingPrinter printer;
    fun->Print(&printer);
    
    std::string expect_file = suit_path_ + "/tests/" + name;
    if (auto rs = env_->FileExists(expect_file); !rs) {
        std::unique_ptr<WritableFile> file;
        rs = env_->NewWritableFile(expect_file + ".tmp", false, &file);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        rs = file->Append(printer.buffer());
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        FAIL() << "Expect file: " << expect_file + " not found";
    } else {
        std::unique_ptr<SequentialFile> file;
        rs = env_->NewSequentialFile(expect_file, &file);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        uint64_t size = 0;
        rs = file->GetFileSize(&size);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        std::string_view result;
        std::string scratch;
        rs = file->Read(size, &result, &scratch);
        ASSERT_TRUE(rs.ok()) << rs.ToString();
        
        if (printer.buffer().compare(result) != 0) {
            std::unique_ptr<WritableFile> file;
            rs = env_->NewWritableFile(expect_file + ".tmp", false, &file);
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            
            rs = file->Append(printer.buffer());
            ASSERT_TRUE(rs.ok()) << rs.ToString();
            FAIL() << "Assert function fail! " << name << "\n" << printer.buffer();
        }
    }
}

using Value = BytecodeGenerator::Value;

TEST_F(BytecodeGeneratorTest, Sanity) {
    Define("011-generator-sanity");
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto err = Parse();
    ASSERT_TRUE(err.ok()) << err.ToString();
    ASSERT_TRUE(generator_->Prepare());
    
    auto value = generator_->FindValue("main.a");
    ASSERT_EQ(Value::kGlobal, value.linkage);
    ASSERT_EQ(isolate_->builtin_type(kType_string), value.type) << value.type->name();

    value = generator_->FindValue("main.b");
    ASSERT_EQ(Value::kGlobal, value.linkage);
    ASSERT_EQ(isolate_->metadata_space()->FindClassOrNull("foo.Foo"), value.type) << value.type->name();
    
    value = generator_->FindValue("runtime.VERSION");
    ASSERT_EQ(Value::kGlobal, value.linkage);
    ASSERT_EQ(isolate_->builtin_type(kType_string), value.type) << value.type->name();
    
    ASSERT_TRUE(generator_->Generate());
    
    value = generator_->FindValue("main.main");
    Local<Closure> main(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(main.is_value_not_null());
    ASSERT_TRUE(main->is_mai_function());
    ASSERT_FALSE(main->is_cxx_function());
    
    value = generator_->FindValue("main.bar");
    Local<Closure> bar(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(bar.is_value_not_null());
    ASSERT_TRUE(bar->is_mai_function());
    ASSERT_FALSE(bar->is_cxx_function());
    
    AssertFunction("init0", generator_->generated_init0_fun());
    AssertFunction("main", main->function());
    AssertFunction("bar", bar->function());
    auto clazz = isolate_->metadata_space()->FindClassOrNull("foo.Foo");
    AssertFunction("Foo_init", clazz->init()->fn()->function());
}

TEST_F(BytecodeGeneratorTest, DemoRun) {
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto rs = isolate_->Compile("tests/lang/011-generator-sanity");
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    isolate_->Run();
}

TEST_F(BytecodeGeneratorTest, StringTemplate) {
    Define("012-string-template");
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto err = Parse();
    ASSERT_TRUE(err.ok()) << err.ToString();
    ASSERT_TRUE(generator_->Prepare());
    ASSERT_TRUE(generator_->Generate());
    
    auto value = generator_->FindValue("main.main");
    Local<Closure> main(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(main.is_value_not_null());
    ASSERT_TRUE(main->is_mai_function());
    ASSERT_FALSE(main->is_cxx_function());

    AssertFunction("main", main->function());
}

TEST_F(BytecodeGeneratorTest, RunStringTemplate) {
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto rs = isolate_->Compile("tests/lang/012-string-template");
    ASSERT_TRUE(rs.ok()) << rs.ToString();

    //auto jiffies = env_->CurrentTimeMicros();
    isolate_->Run();
    //printf("%f\n", (env_->CurrentTimeMicros() - jiffies)/1000.0);
}

// 014-embed-function-and-lambda
TEST_F(BytecodeGeneratorTest, EmbedFunctionAndLambda) {
    Define("014-embed-function-and-lambda");
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto err = Parse();
    ASSERT_TRUE(err.ok()) << err.ToString();
    ASSERT_TRUE(generator_->Prepare());
    ASSERT_TRUE(generator_->Generate());

    auto value = generator_->FindValue("main.main");
    Local<Closure> main(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(main.is_value_not_null());
    ASSERT_TRUE(main->is_mai_function());
    ASSERT_FALSE(main->is_cxx_function());
    
    value = generator_->FindValue("main.bar");
    Local<Closure> bar(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(bar.is_value_not_null());
    ASSERT_TRUE(bar->is_mai_function());
    ASSERT_FALSE(bar->is_cxx_function());
    
    value = generator_->FindValue("main.Foo::doIt");
    Local<Closure> do_it(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(do_it.is_value_not_null());
    ASSERT_TRUE(do_it->is_mai_function());
    ASSERT_FALSE(do_it->is_cxx_function());
    
    value = generator_->FindValue("main.Foo::doThat");
    ASSERT_NE(Value::kError, value.linkage);
    Local<Closure> do_that(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(do_that.is_value_not_null());
    ASSERT_TRUE(do_that->is_mai_function());
    ASSERT_FALSE(do_that->is_cxx_function());

    AssertFunction("main", main->function());
    AssertFunction("bar", bar->function());
    AssertFunction("do_it", do_it->function());
    AssertFunction("do_that", do_that->function());
}

TEST_F(BytecodeGeneratorTest, RunEmbedFunctionAndLambda) {
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto rs = isolate_->Compile("tests/lang/014-embed-function-and-lambda");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    //auto jiffies = env_->CurrentTimeMicros();
    isolate_->Run();
    //printf("%f\n", (env_->CurrentTimeMicros() - jiffies)/1000.0);
}

TEST_F(BytecodeGeneratorTest, ThrowCatchException) {
    Define("015-throw-catch-exception");
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto err = Parse();
    ASSERT_TRUE(err.ok()) << err.ToString();
    ASSERT_TRUE(generator_->Prepare());
    ASSERT_TRUE(generator_->Generate());
    
    //base::StdFilePrinter printer(stdout);
    
    auto value = generator_->FindValue("main.main");
    Local<Closure> main(*isolate_->global_offset<Closure *>(value.index));
    ASSERT_TRUE(main.is_value_not_null());
    ASSERT_TRUE(main->is_mai_function());
    ASSERT_FALSE(main->is_cxx_function());
    
    AssertFunction("main", main->function());
    
    auto clazz = isolate_->metadata_space()->FindClassOrNull("main.MyException");
    ASSERT_NE(nullptr, clazz);
    AssertFunction("main_MyException_init", clazz->init()->fn()->function());
    
    clazz = isolate_->metadata_space()->FindClassOrNull("lang.Exception");
    ASSERT_NE(nullptr, clazz);
    AssertFunction("lang_Exception_init", clazz->init()->fn()->function());
}

TEST_F(BytecodeGeneratorTest, RunThrowCatchException) {
    HandleScope handle_scope(HandleScope::INITIALIZER);

    auto rs = isolate_->Compile("tests/lang/015-throw-catch-exception");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    //auto jiffies = env_->CurrentTimeMicros();
    isolate_->Run();
    //printf("%f\n", (env_->CurrentTimeMicros() - jiffies)/1000.0);
}

// 015-throw-catch-exception

} // namespace lang

} // namespace mai
