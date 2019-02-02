#include "sql/parser.h"
#include "sql/ast.h"
#include "sql/parser-ctx.h"
#include "sql/sql.hh"
#include "sql/sql.yy.h"
#include "sql/ast-factory.h"
#include "base/arena-utils.h"
#include "base/slice.h"

namespace mai {
    
namespace sql {
    
namespace ast {
    
Location::Location(const YYLTYPE &yyl)
    : begin_line(yyl.first_line)
    , begin_column(yyl.first_column)
    , end_line(yyl.last_line)
    , end_column(yyl.last_column) {
}
    
/*static*/ Location Location::Concat(const YYLTYPE &first, const YYLTYPE &last) {
    Location result(first);
    result.end_line   = last.last_line;
    result.end_column = last.last_column;
    return result;
}

} // namespace ast
    
std::string Parser::Result::FormatError() {
    if (!error || error == AstString::kEmpty) {
        return "";
    }
    return base::Slice::Sprintf("[%d,%d] %s", line, column, error->data());
}


/*static*/ Error
Parser::Parse(const char *s, base::Arena *arena, Parser::Result *result) {
    ast::Factory factory(arena);
    parser_ctx ctx;
    ctx.factory = &factory;
    
    lexer_extra extra;
    yylex_init_extra(&extra, &ctx.lex);
    auto buf = yy_scan_string(s, ctx.lex);
    yy_switch_to_buffer(buf, ctx.lex);
    
    yyparse(&ctx);
    
    yy_delete_buffer(buf, ctx.lex);
    yylex_destroy(ctx.lex);
    
    result->ast    = ctx.ast;
    result->block  = ctx.block;
    result->error  = ctx.err_msg;
    result->line   = ctx.err_line;
    result->column = ctx.err_column;
    return ctx.err_msg ? MAI_CORRUPTION("Parse fail!") : Error::OK();
}

/*static*/ Error Parser::Parse(const char *s, size_t n, base::Arena *arena,
                               Result *result) {
    ast::Factory factory(arena);
    parser_ctx ctx;
    ctx.factory = &factory;
    
    lexer_extra extra;
    yylex_init_extra(&extra, &ctx.lex);
    auto buf = yy_scan_bytes(s, n, ctx.lex);
    yy_switch_to_buffer(buf, ctx.lex);
    
    yyparse(&ctx);
    
    yy_delete_buffer(buf, ctx.lex);
    yylex_destroy(ctx.lex);
    
    result->ast    = ctx.ast;
    result->block  = ctx.block;
    result->error  = ctx.err_msg;
    result->line   = ctx.err_line;
    result->column = ctx.err_column;
    return ctx.err_msg ? MAI_CORRUPTION("Parse fail!") : Error::OK();
}
    
} // namespace sql
    
} // namespace mai
