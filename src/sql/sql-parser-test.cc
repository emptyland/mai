#include "sql/parser-ctx.h"
#include "sql/sql.hh"
extern "C" {
#include "sql/sql.yy.h"
}
#include "gtest/gtest.h"

namespace mai {
    
namespace sql {

class SQLParserTest : public ::testing::Test {
public:
    
};
    
TEST_F(SQLParserTest, YaccTest) {
//    parser_ctx ctx;
//    yylex_init(&ctx.lex);
//    yyset_in(stdin, ctx.lex);
//
//    yyparse(&ctx);
//
//    yylex_destroy(ctx.lex);
}

} // namespace sql
    
} // namespace mai
