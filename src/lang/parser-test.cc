#include "lang/parser.h"
#include "lang/lexer.h"
#include "lang/ast.h"
#include "lang/syntax-mock-test.h"
#include "base/arena-utils.h"
#include "base/arenas.h"
#include "gtest/gtest.h"

namespace mai {

namespace lang {

class ParserTest : public ::testing::Test {
public:
    ParserTest()
        : parser_(&arena_, &feedback_) {}
    
    MockFeedback feedback_;
    base::StandaloneArena arena_;
    Parser parser_;
};

TEST_F(ParserTest, Sanity) {
    MockFile file("package demo");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    bool ok = true;
    FileUnit *ast = parser_.Parse(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ("demos/demo.mai", ast->file_name()->ToString());
    ASSERT_EQ("demo", ast->package_name()->ToString());
}

TEST_F(ParserTest, Import) {
    MockFile file("package demo\n"
                  "import testing.foo.bar.baz\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    FileUnit *ast = parser_.Parse(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(1, ast->import_packages_size());
    auto stmt = ast->import_package(0);
    ASSERT_EQ("testing.foo.bar.baz", stmt->original_path()->ToString());
    ASSERT_EQ(nullptr, stmt->alias());
}

TEST_F(ParserTest, ImportBlock) {
    MockFile file("package demo\n"
                  "import {\n"
                  "    testing.foo as Foo\n"
                  "    testing.bar as Bar\n"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    FileUnit *ast = parser_.Parse(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(2, ast->import_packages_size());
    auto stmt = ast->import_package(0);
    ASSERT_EQ("testing.foo", stmt->original_path()->ToString());
    ASSERT_EQ("Foo", stmt->alias()->ToString());
    
    stmt = ast->import_package(1);
    ASSERT_EQ("testing.bar", stmt->original_path()->ToString());
    ASSERT_EQ("Bar", stmt->alias()->ToString());
}

TEST_F(ParserTest, ValDeclaration) {
    MockFile file("var a: int\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseVariableDeclaration(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_STREQ("a", ast->identifier()->data());
    ASSERT_EQ(VariableDeclaration::VAR, ast->kind());
    ASSERT_EQ(Token::kInt, ast->type()->id());

    MockFile file1("val a = 1\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);
    // var a: func (int, int): void
    ast = parser_.ParseVariableDeclaration(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_STREQ("a", ast->identifier()->data());
    ASSERT_EQ(VariableDeclaration::VAL, ast->kind());
    ASSERT_EQ(nullptr, ast->type());
    ASSERT_NE(nullptr, ast->initializer());
    ASSERT_TRUE(ast->initializer()->IsIntLiteral());
    ASSERT_EQ(1, ast->initializer()->AsIntLiteral()->value());
}

TEST_F(ParserTest, FunctionTypeSign) {
    MockFile file("fun (int, ...): string\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseTypeSign(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto proto = ast->prototype();
    ASSERT_TRUE(proto->vargs());
    ASSERT_EQ(Token::kString, proto->return_type()->id());
    ASSERT_EQ(1, proto->parameters_size());
    ASSERT_EQ(Token::kInt, proto->parameter(0).type->id());
}

TEST_F(ParserTest, PairExpression) {
    MockFile file("(a -> b)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    auto pair = ast->AsPairExpression();
    ASSERT_NE(nullptr, pair);
    ASSERT_FALSE(pair->addition_key());
    ASSERT_STREQ("a", pair->key()->AsIdentifier()->name()->data());
    ASSERT_STREQ("b", pair->value()->AsIdentifier()->name()->data());
    
    MockFile file1("(.. -> b)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    pair = ast->AsPairExpression();
    ASSERT_NE(nullptr, pair);
    ASSERT_TRUE(pair->addition_key());
    ASSERT_STREQ("b", pair->value()->AsIdentifier()->name()->data());
}

TEST_F(ParserTest, ParenExpression) {
    MockFile file("c * (a + b)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    auto bin = ast->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("c", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kMul, bin->op().kind);
    
    bin = bin->rhs()->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("a", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_STREQ("b", bin->rhs()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kAdd, bin->op().kind);
}

TEST_F(ParserTest, IncrementExpression) {
    MockFile file("a + a++\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto bin = ast->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("a", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kAdd, bin->op().kind);

    auto una = bin->rhs()->AsUnaryExpression();
    ASSERT_NE(nullptr, una);
    ASSERT_STREQ("a", una->operand()->AsIdentifier()->name()->data());
    ASSERT_EQ(Operator::kIncrementPost, una->op().kind);
}

TEST_F(ParserTest, AssignmentStatement) {
    MockFile file("a += (.. -> 100)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseAssignmentOrExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    auto ass = ast->AsAssignmentStatement();
    ASSERT_NE(nullptr, ass);
    
    ASSERT_EQ(Operator::kAdd, ass->assignment_op().kind);
    ASSERT_STREQ("a", ass->lval()->AsIdentifier()->name()->data());
    auto pair = ass->rval()->AsPairExpression();
    ASSERT_NE(nullptr, pair);
    ASSERT_TRUE(pair->addition_key());
    ASSERT_EQ(100, pair->value()->AsIntLiteral()->value());
}

TEST_F(ParserTest, NativeFunctionDeclaration) {
    MockFile file("native fun foo(a:int, b:int)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseFunctionDefinition(0, &ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_TRUE(ast->is_native());
    ASSERT_STREQ("foo", ast->identifier()->data());
    ASSERT_EQ(2, ast->prototype()->parameters_size());
    ASSERT_STREQ("a", ast->prototype()->parameter(0).name->data());
    ASSERT_EQ(Token::kInt, ast->prototype()->parameter(0).type->id());
    ASSERT_STREQ("b", ast->prototype()->parameter(1).name->data());
    ASSERT_EQ(Token::kInt, ast->prototype()->parameter(1).type->id());
    ASSERT_EQ(Token::kVoid, ast->prototype()->return_type()->id());
}

TEST_F(ParserTest, FunctionDefinition) {
    MockFile file("fun foo(a:int, b:int): int { return a + b }\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseFunctionDefinition(0, &ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(1, ast->statements_size());
    auto ret = ast->statement(0)->AsBreakableStatement();
    ASSERT_TRUE(ret->IsReturn());
    auto bin = ret->value()->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_STREQ("a", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_STREQ("b", bin->rhs()->AsIdentifier()->name()->data());
}

TEST_F(ParserTest, ClassDefinition) {
    MockFile file("class Foo { val i = 1 }\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    bool ok = true;
    auto ast = parser_.ParseClassDefinition(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_STREQ("Foo", ast->identifier()->data());
    ASSERT_EQ(1, ast->fields_size());
    ASSERT_FALSE(ast->field(0).in_constructor);
    ASSERT_EQ(ClassDefinition::kPublic, ast->field(0).access);
    ASSERT_EQ(VariableDeclaration::VAL, ast->field(0).declaration->kind());
    ASSERT_STREQ("i", ast->field(0).declaration->identifier()->data());
    ASSERT_EQ(1, ast->field(0).declaration->initializer()->AsIntLiteral()->value());

    MockFile file1("class Foo(\n"
                   "private:\n"
                   "    val a: int,\n"
                   "public:\n"
                   "    val b: int,\n"
                   "    c: int,\n"
                   "    d: int\n"
                   "): foo.Bar(c, d) {}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);
    
    ok = true;
    ast = parser_.ParseClassDefinition(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_STREQ("Foo", ast->identifier()->data());
    ASSERT_EQ(4, ast->parameters_size());

    ASSERT_TRUE(ast->parameter(0).field_declaration);
    ASSERT_EQ(0, ast->parameter(0).as_field);

    ASSERT_TRUE(ast->parameter(1).field_declaration);
    ASSERT_EQ(1, ast->parameter(1).as_field);
    
    ASSERT_FALSE(ast->parameter(2).field_declaration);
    ASSERT_STREQ("c", ast->parameter(2).as_parameter->identifier()->data());
    ASSERT_EQ(Token::kInt, ast->parameter(2).as_parameter->type()->id());
    
    ASSERT_FALSE(ast->parameter(3).field_declaration);
    ASSERT_STREQ("d", ast->parameter(3).as_parameter->identifier()->data());
    ASSERT_EQ(Token::kInt, ast->parameter(3).as_parameter->type()->id());
    
    ASSERT_STREQ("Bar", ast->base_name()->data());
    ASSERT_STREQ("foo", ast->base_prefix()->data());
    ASSERT_EQ(2, ast->arguments_size());
    ASSERT_STREQ("c", ast->argument(0)->AsIdentifier()->name()->data());
    ASSERT_STREQ("d", ast->argument(1)->AsIdentifier()->name()->data());
    
    ASSERT_EQ(2, ast->fields_size());
    ASSERT_EQ(ClassDefinition::kPrivate, ast->field(0).access);
    ASSERT_TRUE(ast->field(0).in_constructor);
    ASSERT_EQ(0, ast->field(0).as_constructor);
    ASSERT_STREQ("a", ast->field(0).declaration->identifier()->data());
    ASSERT_EQ(Token::kInt, ast->field(0).declaration->type()->id());
    
    ASSERT_EQ(ClassDefinition::kPublic, ast->field(1).access);
    ASSERT_TRUE(ast->field(1).in_constructor);
    ASSERT_EQ(1, ast->field(1).as_constructor);
    ASSERT_STREQ("b", ast->field(1).declaration->identifier()->data());
    ASSERT_EQ(Token::kInt, ast->field(1).declaration->type()->id());
}

TEST_F(ParserTest, ObjectDefinition) {
    MockFile file("object Foo { val i = 1 }\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseObjectDefinition(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_STREQ("Foo", ast->identifier()->data());
    ASSERT_EQ(1, ast->fields_size());
    ASSERT_STREQ("i", ast->field(0).declaration->identifier()->data());
    ASSERT_FALSE(ast->field(0).in_constructor);
    ASSERT_EQ(1, ast->field(0).declaration->initializer()->AsIntLiteral()->value());
}

TEST_F(ParserTest, InterfaceDefinition) {
    MockFile file("interface Foo {\n"
                  "    doIt(i:int, j:string)\n"
                  "    doThat(): int\n"
                  "    doThis(m:string)\n"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseInterfaceDefinition(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_STREQ("Foo", ast->identifier()->data());
    ASSERT_EQ(3, ast->methods_size());
    ASSERT_STREQ("doIt", ast->method(0)->identifier()->data());
    ASSERT_STREQ("doThat", ast->method(1)->identifier()->data());
    ASSERT_STREQ("doThis", ast->method(2)->identifier()->data());
    
    auto method = ast->FindMethodOrNull("doIt");
    ASSERT_NE(nullptr, method);
    ASSERT_TRUE(method->is_declare());
    
    method = ast->FindMethodOrNull("doThat");
    ASSERT_NE(nullptr, method);
    ASSERT_TRUE(method->is_declare());
    
    method = ast->FindMethodOrNull("doThis");
    ASSERT_NE(nullptr, method);
    ASSERT_TRUE(method->is_declare());
}

TEST_F(ParserTest, ClassImplementsBlock) {
    MockFile file("implements Foo {\n"
                  "    native fun doIt()\n"
                  "    native fun doThat(a:int): string\n"
                  "    fun doThis(a:int, b:int): int { return a + b }\n"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseClassImplementsBlock(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(3, ast->methods_size());
    ASSERT_STREQ("doIt", ast->method(0)->identifier()->data());
    ASSERT_TRUE(ast->method(0)->is_native());
    ASSERT_EQ(0, ast->method(0)->prototype()->parameters_size());
    
    ASSERT_STREQ("doThat", ast->method(1)->identifier()->data());
    ASSERT_TRUE(ast->method(1)->is_native());
    ASSERT_EQ(1, ast->method(1)->parameters_size());
    ASSERT_STREQ("a", ast->method(1)->parameter(0).name->data());
    ASSERT_EQ(Token::kInt, ast->method(1)->parameter(0).type->id());
    
    ASSERT_STREQ("doThis", ast->method(2)->identifier()->data());
    ASSERT_FALSE(ast->method(2)->is_native());
    ASSERT_EQ(2, ast->method(2)->parameters_size());
    ASSERT_STREQ("a", ast->method(2)->parameter(0).name->data());
    ASSERT_EQ(Token::kInt, ast->method(2)->parameter(0).type->id());
    ASSERT_STREQ("b", ast->method(2)->parameter(1).name->data());
    ASSERT_EQ(Token::kInt, ast->method(2)->parameter(1).type->id());
}

TEST_F(ParserTest, ArrayInitializer) {
    MockFile file("array[int](100)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    bool ok = true;
    auto ast = parser_.ParseArrayInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(Token::kInt, ast->element_type()->id());
    ASSERT_EQ(100, ast->reserve()->AsIntLiteral()->value());
}

TEST_F(ParserTest, MapInitializer) {
    MockFile file("map[string, int](100)\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseMapInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_FALSE(ast->mutable_container());
    ASSERT_EQ(Token::kString, ast->key_type()->id());
    ASSERT_EQ(Token::kInt, ast->value_type()->id());
    ASSERT_EQ(100, ast->reserve()->AsIntLiteral()->value());

    MockFile file1("map{a+b->1, \"ok\"->a/b}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ok = true;
    ast = parser_.ParseMapInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_FALSE(ast->mutable_container());
    ASSERT_EQ(nullptr, ast->key_type());
    ASSERT_EQ(nullptr, ast->value_type());
    ASSERT_EQ(2, ast->operands_size());
    ASSERT_TRUE(ast->operand(0)->IsPairExpression());
    ASSERT_TRUE(ast->operand(1)->IsPairExpression());
}

TEST_F(ParserTest, LambdaLiteral) {
    MockFile file("λ (a: int, b: int) { return a + b }\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    // val i = ◁ ch
    // ch ◁ i
    bool ok = true;
    auto ast = parser_.ParseLambdaLiteral(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
}

TEST_F(ParserTest, ChannelRecvSend) {
    MockFile file("<- ch\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto unary = ast->AsUnaryExpression();
    ASSERT_NE(nullptr, unary);
    ASSERT_EQ(Operator::kRecv, unary->op().kind);
    ASSERT_STREQ("ch", unary->operand()->AsIdentifier()->name()->data());

    MockFile file1("ch <- data\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ok = true;
    ast = parser_.ParseExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    auto bin = ast->AsBinaryExpression();
    ASSERT_NE(nullptr, bin);
    ASSERT_EQ(Operator::kSend, bin->op().kind);
    ASSERT_STREQ("ch", bin->lhs()->AsIdentifier()->name()->data());
    ASSERT_STREQ("data", bin->rhs()->AsIdentifier()->name()->data());
}

TEST_F(ParserTest, IfExpression) {
    MockFile file("if (ok) 1\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseIfExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(nullptr, ast->extra_statement());
    ASSERT_TRUE(ast->condition()->IsIdentifier());
    ASSERT_STREQ("ok", ast->condition()->AsIdentifier()->name()->data());
    ASSERT_TRUE(ast->branch_true()->IsIntLiteral());
    ASSERT_EQ(1, ast->branch_true()->AsIntLiteral()->value());
    ASSERT_EQ(nullptr, ast->branch_false());
    
    MockFile file1("if (val i = 1; ok) 1 else 2\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ok = true;
    ast = parser_.ParseIfExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_NE(nullptr, ast->extra_statement());
    ASSERT_TRUE(ast->extra_statement()->IsVariableDeclaration());
    ASSERT_STREQ("i", ast->extra_statement()->AsVariableDeclaration()->identifier()->data());
    ASSERT_EQ(1, ast->extra_statement()->AsVariableDeclaration()->initializer()->AsIntLiteral()->value());
    ASSERT_TRUE(ast->branch_true()->IsIntLiteral());
    ASSERT_EQ(1, ast->branch_true()->AsIntLiteral()->value());
    ASSERT_TRUE(ast->branch_false()->IsIntLiteral());
    ASSERT_EQ(2, ast->branch_false()->AsIntLiteral()->value());
}

TEST_F(ParserTest, IfElseIfExpression) {
    MockFile file("if (a) 1 else if (b) 2 else if (c) 3 else 4\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseIfExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(nullptr, ast->extra_statement());
    ASSERT_TRUE(ast->condition()->IsIdentifier());
    ASSERT_STREQ("a", ast->condition()->AsIdentifier()->name()->data());
    ASSERT_TRUE(ast->branch_true()->IsIntLiteral());
    ASSERT_EQ(1, ast->branch_true()->AsIntLiteral()->value());
    ASSERT_TRUE(ast->branch_false()->IsIfExpression());
    
    ast = ast->branch_false()->AsIfExpression();
    ASSERT_EQ(nullptr, ast->extra_statement());
    ASSERT_TRUE(ast->condition()->IsIdentifier());
    ASSERT_STREQ("b", ast->condition()->AsIdentifier()->name()->data());
    ASSERT_TRUE(ast->branch_true()->IsIntLiteral());
    ASSERT_EQ(2, ast->branch_true()->AsIntLiteral()->value());
    ASSERT_TRUE(ast->branch_false()->IsIfExpression());
    
    ast = ast->branch_false()->AsIfExpression();
    ASSERT_EQ(nullptr, ast->extra_statement());
    ASSERT_TRUE(ast->condition()->IsIdentifier());
    ASSERT_STREQ("c", ast->condition()->AsIdentifier()->name()->data());
    ASSERT_TRUE(ast->branch_true()->IsIntLiteral());
    ASSERT_EQ(3, ast->branch_true()->AsIntLiteral()->value());
    ASSERT_TRUE(ast->branch_false()->IsIntLiteral());
    ASSERT_EQ(4, ast->branch_false()->AsIntLiteral()->value());
}

TEST_F(ParserTest, WhileLoop) {
    MockFile file("while (ok) {}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseWhileLoop(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_TRUE(ast->condition()->IsIdentifier());
    ASSERT_STREQ("ok", ast->condition()->AsIdentifier()->name()->data());
    ASSERT_EQ(0, ast->statements_size());
    
    MockFile file1("unless (ok) {val i = 1}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file1);

    ok = true;
    ast = parser_.ParseWhileLoop(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_TRUE(ast->condition()->IsUnaryExpression());
    ASSERT_EQ(Operator::kNot, ast->condition()->AsUnaryExpression()->op().kind);
    ASSERT_STREQ("ok", ast->condition()->AsUnaryExpression()->operand()->AsIdentifier()->name()->data());
    ASSERT_EQ(1, ast->statements_size());
    ASSERT_STREQ("i", ast->statement(0)->AsVariableDeclaration()->identifier()->data());
    ASSERT_EQ(1, ast->statement(0)->AsVariableDeclaration()->initializer()->AsIntLiteral()->value());
}

TEST_F(ParserTest, ForStepLoop) {
    MockFile file("for (i in 0, 1000) {}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseForLoop(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    ASSERT_EQ(ForLoop::STEP, ast->control());
    ASSERT_EQ(nullptr, ast->key());
    ASSERT_NE(nullptr, ast->value());
    ASSERT_STREQ("i", ast->value()->identifier()->data());
    ASSERT_TRUE(ast->subject()->IsIntLiteral());
    ASSERT_EQ(0, ast->subject()->AsIntLiteral()->value());
    ASSERT_TRUE(ast->limit()->IsIntLiteral());
    ASSERT_EQ(1000, ast->limit()->AsIntLiteral()->value());
}

TEST_F(ParserTest, ForContainerIterateLoop) {
    MockFile file("for (key, value in arr) {}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseForLoop(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    ASSERT_EQ(ForLoop::EACH, ast->control());
    ASSERT_NE(nullptr, ast->key());
    ASSERT_NE(nullptr, ast->value());
    ASSERT_STREQ("key", ast->key()->identifier()->data());
    ASSERT_STREQ("value", ast->value()->identifier()->data());
}

TEST_F(ParserTest, TryCatchFinallyBlock) {
    MockFile file("try {\n"
                  "    throw a\n"
                  "} catch(e: Exception) {\n"
                  "    e.printStackstrace()\n"
                  "} catch(e: Error) {\n"
                  "    e.printStackstrace()\n"
                  "} finally {\n"
                  "    clean()"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseTryCatchFinallyBlock(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(1, ast->try_statements_size());
    ASSERT_EQ(2, ast->catch_blocks_size());
    ASSERT_EQ(1, ast->finally_statements_size());
}

// val ch = channel[int](1)
// val n = <-ch
// ch <- 1
TEST_F(ParserTest, ChannelInitializer) {
    MockFile file("channel[int](0)");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseChannelInitializer(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_EQ(Token::kInt, ast->value_type()->id());
    ASSERT_EQ(0, ast->buffer_size()->AsIntLiteral()->value());
}

TEST_F(ParserTest, TypeCastWhenExpression) {
    MockFile file("when(a) {\n"
                  "    b: int -> b + 1\n"
                  "    b: uint -> b + 1u\n"
                  "    b: string -> \"b=$b\"\n"
                  "    else -> nil\n"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseWhenExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_NE(nullptr, ast->primary());
    ASSERT_TRUE(ast->primary()->IsIdentifier());
    ASSERT_STREQ("a", ast->primary()->AsIdentifier()->name()->data());
    ASSERT_EQ(3, ast->clauses_size());
    ASSERT_NE(nullptr, ast->else_clause());
    ASSERT_TRUE(ast->else_clause()->IsNilLiteral());
    
    auto clause = ast->clause(0);
    ASSERT_TRUE(clause.single_case->IsVariableDeclaration());
    ASSERT_STREQ("b", clause.single_case->AsVariableDeclaration()->identifier()->data());
    ASSERT_EQ(Token::kInt, clause.single_case->AsVariableDeclaration()->type()->id());
    ASSERT_NE(nullptr, clause.body);
    
    clause = ast->clause(1);
    ASSERT_TRUE(clause.single_case->IsVariableDeclaration());
    ASSERT_STREQ("b", clause.single_case->AsVariableDeclaration()->identifier()->data());
    ASSERT_EQ(Token::kUInt, clause.single_case->AsVariableDeclaration()->type()->id());
    ASSERT_NE(nullptr, clause.body);
}

TEST_F(ParserTest, SwitchWhenExpression) {
    MockFile file("when(a) {\n"
                  "    1 -> 'byte'\n"
                  "    2 -> 'word'\n"
                  "    4 -> 'dword'\n"
                  "    5,6,7 -> 'huge'\n"
                  "    else -> 'unknown'\n"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);

    bool ok = true;
    auto ast = parser_.ParseWhenExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);

    ASSERT_NE(nullptr, ast->primary());
    ASSERT_TRUE(ast->primary()->IsIdentifier());
    ASSERT_STREQ("a", ast->primary()->AsIdentifier()->name()->data());
    
    ASSERT_EQ(4, ast->clauses_size());
    
    ASSERT_FALSE(ast->clause(0).is_multi);
    auto expect = ast->clause(0).single_case;
    ASSERT_TRUE(expect->IsIntLiteral());
    ASSERT_EQ(1, expect->AsIntLiteral()->value());
    auto value = ast->clause(0).body;
    ASSERT_TRUE(value->IsStringLiteral());
    ASSERT_STREQ("byte", value->AsStringLiteral()->value()->data());
    
    ASSERT_FALSE(ast->clause(1).is_multi);
    expect = ast->clause(1).single_case;
    ASSERT_TRUE(expect->IsIntLiteral());
    ASSERT_EQ(2, expect->AsIntLiteral()->value());
    value = ast->clause(1).body;
    ASSERT_TRUE(value->IsStringLiteral());
    ASSERT_STREQ("word", value->AsStringLiteral()->value()->data());
    
    ASSERT_FALSE(ast->clause(2).is_multi);
    expect = ast->clause(2).single_case;
    ASSERT_TRUE(expect->IsIntLiteral());
    ASSERT_EQ(4, expect->AsIntLiteral()->value());
    value = ast->clause(2).body;
    ASSERT_TRUE(value->IsStringLiteral());
    ASSERT_STREQ("dword", value->AsStringLiteral()->value()->data());
    
    ASSERT_TRUE(ast->clause(3).is_multi);
    ASSERT_EQ(3, ast->clause(3).multi_cases->size());
    value = ast->clause(3).body;
    ASSERT_TRUE(value->IsStringLiteral());
    ASSERT_STREQ("huge", value->AsStringLiteral()->value()->data());
}

TEST_F(ParserTest, ConditionWhenExpression) {
    MockFile file("when {\n"
                  "    a > 1 -> 'greater'\n"
                  "    a < 1 -> 'less'\n"
                  "    a == 1 -> 'equal'\n"
                  "    else -> 'unknown'\n"
                  "}\n");
    parser_.SwitchInputFile("demos/demo.mai", &file);
    
    bool ok = true;
    auto ast = parser_.ParseWhenExpression(&ok);
    ASSERT_TRUE(ok);
    ASSERT_NE(nullptr, ast);
    
    ASSERT_EQ(nullptr, ast->primary());
    ASSERT_EQ(3, ast->clauses_size());
    
    auto cond = ast->clause(0).single_case;
    ASSERT_TRUE(cond->IsBinaryExpression());
    ASSERT_EQ(Operator::kGreater, cond->AsBinaryExpression()->op().kind);
    auto value = ast->clause(0).body;
    ASSERT_TRUE(value->IsStringLiteral());
    ASSERT_STREQ("greater", value->AsStringLiteral()->value()->data());
    
    cond = ast->clause(1).single_case;
    ASSERT_TRUE(cond->IsBinaryExpression());
    ASSERT_EQ(Operator::kLess, cond->AsBinaryExpression()->op().kind);
    value = ast->clause(1).body;
    ASSERT_TRUE(value->IsStringLiteral());
    ASSERT_STREQ("less", value->AsStringLiteral()->value()->data());
}

} // namespace lang

} // namespace mai
