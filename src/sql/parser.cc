#include "sql/parser.h"
#include "sql/parser-ctx.h"
#include "sql/sql.hh"
extern "C" {
#include "sql/sql.yy.h"
}
#include "base/arena-utils.h"

namespace mai {
    
namespace sql {
    
    
/*static*/ Error
Parser::Parse(const char *s, AstFactory *factory, Parser::Result *result) {
    parser_ctx ctx;
    ctx.factory = factory;
    
    yylex_init(&ctx.lex);
    auto buf = yy_scan_string(s, ctx.lex);
    yy_switch_to_buffer(buf, ctx.lex);
    
    yyparse(&ctx);
    
    yy_delete_buffer(buf, ctx.lex);
    yylex_destroy(ctx.lex);
    
    result->ast    = ctx.ast;
    result->error  = ctx.err_msg;
    result->line   = ctx.err_line;
    result->column = ctx.err_column;
    return ctx.err_msg ? MAI_CORRUPTION("Parse fail!") : Error::OK();
}

    
} // namespace sql
    
} // namespace mai
