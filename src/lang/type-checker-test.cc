#include "lang/compiler.h"
#include "lang/type-checker.h"
#include "lang/lexer.h"
#include "lang/ast.h"
#include "lang/syntax-mock-test.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class TypeCheckerTest : public ::testing::Test {
public:
    TypeCheckerTest()
        : env_(Env::Default())
        , resolver_(env_, &arena_, &feedback_, {})
        , checker_(&arena_, &feedback_) {}
    
    Error Parse(const std::string &dir) {
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
        return checker_.AddBootFileUnits(dir, units, &resolver_);
    }
    
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Env *env_;
    SourceFileResolve resolver_;
    TypeChecker checker_;
};

TEST_F(TypeCheckerTest, Sanity) {
    static constexpr char kPath[] = "tests/lang/000-type-checker-sanity";
    
    auto rs = Parse(kPath);
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    
    auto path_units = checker_.FindPathUnits("foo");
    ASSERT_EQ(1, path_units.size());
    
    path_units = checker_.FindPathUnits("mai.lang");
    ASSERT_LE(1, path_units.size());
    
    path_units = checker_.FindPathUnits("mai.runtime");
    ASSERT_LE(1, path_units.size());
    
    path_units = checker_.FindPackageUnits("foo");
    ASSERT_EQ(1, path_units.size());
    
    path_units = checker_.FindPackageUnits("main");
    ASSERT_EQ(1, path_units.size());
}

TEST_F(TypeCheckerTest, Prepare) {
    auto rs = Parse("tests/lang/000-type-checker-sanity");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    
    auto sym = checker_.FindSymbolOrNull("lang.println");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsFunctionDefinition());
    ASSERT_STREQ("println", sym->AsFunctionDefinition()->identifier()->data());
    ASSERT_TRUE(sym->AsFunctionDefinition()->is_native());
    
    sym = checker_.FindSymbolOrNull("foo.i");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    
    sym = checker_.FindSymbolOrNull("main.i");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    
    sym = checker_.FindSymbolOrNull("main.main");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsFunctionDefinition());
    ASSERT_FALSE(sym->AsFunctionDefinition()->is_native());
}

TEST_F(TypeCheckerTest, SimpleGlobalVariableTypeReduce) {
    auto rs = Parse("tests/lang/001-type-checker-variable");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.i");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    ASSERT_STREQ("i", sym->identifier()->data());
    ASSERT_EQ(Token::kInt, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.j");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    ASSERT_STREQ("j", sym->identifier()->data());
    ASSERT_EQ(Token::kF32, sym->AsVariableDeclaration()->type()->id());
}

TEST_F(TypeCheckerTest, TypeNameResolve) {
    auto rs = Parse("tests/lang/002-type-checker-type-resolve");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.o1");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    
    auto var = sym->AsVariableDeclaration();
    ASSERT_NE(nullptr, var);
    ASSERT_EQ(Token::kRef, var->type()->id());
    
    auto clazz = var->type()->clazz();
    ASSERT_NE(nullptr, clazz);
    ASSERT_STREQ("Foo", clazz->identifier()->data());
}

TEST_F(TypeCheckerTest, ClassDefinitionBaseClass) {
    auto rs = Parse("tests/lang/003-type-checker-class-definition");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.Bar");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsClassDefinition());
    
    auto clazz = sym->AsClassDefinition();
    ASSERT_STREQ("Bar", clazz->identifier()->data());
    ASSERT_EQ(Token::kInt, clazz->FindFieldOrNull("max")->declaration->type()->id());
    
    clazz = clazz->base();
    ASSERT_NE(nullptr, clazz);
    ASSERT_STREQ("Foo", clazz->identifier()->data());
}

TEST_F(TypeCheckerTest, ClassDefinitionFieldPerm) {
    auto rs = Parse("tests/lang/004-type-checker-class-perm");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.Foo");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsClassDefinition());
    
    auto clazz = sym->AsClassDefinition();
    ASSERT_NE(nullptr, clazz->base());
    ASSERT_STREQ("Object", clazz->base()->identifier()->data());
}

TEST_F(TypeCheckerTest, InterfaceSanity) {
    auto rs = Parse("tests/lang/005-type-checker-interface");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.Foo::doIt");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsFunctionDefinition());
    
    sym = checker_.FindSymbolOrNull("main.Foo::doThat");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsFunctionDefinition());
    
    sym = checker_.FindSymbolOrNull("main.Foo::doThis");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsFunctionDefinition());
}

TEST_F(TypeCheckerTest, Expressions) {
    auto rs = Parse("tests/lang/006-type-checker-expression");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.c");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kInt, sym->AsVariableDeclaration()->type()->id());
}

TEST_F(TypeCheckerTest, NumberCast) {
    auto rs = Parse("tests/lang/007-type-checker-number-cast");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.b");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kI8, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.c");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kU8, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.d");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kI16, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.e");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kU16, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.f");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kI32, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.g");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kU32, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.h");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kInt, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.i");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kUInt, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.j");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kI64, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.k");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kU64, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.l");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kF32, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.m");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kF64, sym->AsVariableDeclaration()->type()->id());
}

TEST_F(TypeCheckerTest, IfElseExpression) {
    auto rs = Parse("tests/lang/008-type-checker-if-else");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.c");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kString, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("main.e");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kI8, sym->AsVariableDeclaration()->type()->id());
    
    sym = checker_.FindSymbolOrNull("foo.a");
    ASSERT_NE(nullptr, sym);
    ASSERT_EQ(Token::kI8, sym->AsVariableDeclaration()->type()->id());
}

TEST_F(TypeCheckerTest, WhileLoop) {
    auto rs = Parse("tests/lang/009-type-checker-while-loop");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
}

TEST_F(TypeCheckerTest, TryCatchFinally) {
    auto rs = Parse("tests/lang/010-type-checker-try-catch-finally");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.MyException");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsClassDefinition());
    ASSERT_STREQ("Exception", sym->AsClassDefinition()->base()->identifier()->data());
}

TEST_F(TypeCheckerTest, LambdaExpression) {
    auto rs = Parse("tests/lang/013-type-checker-lambda-expr");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
}

TEST_F(TypeCheckerTest, WhenExpression) {
    auto rs = Parse("tests/lang/030-type-checker-when-expression");
    ASSERT_TRUE(rs.ok()) << rs.ToString();
    ASSERT_TRUE(checker_.Prepare());
    ASSERT_TRUE(checker_.Check());
    
    auto sym = checker_.FindSymbolOrNull("main.a");
    ASSERT_NE(nullptr, sym);
    ASSERT_TRUE(sym->IsVariableDeclaration());
    ASSERT_EQ(Token::kString, sym->AsVariableDeclaration()->type()->id());
}

} // namespace lang

} // namespace mai
