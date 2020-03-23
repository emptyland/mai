#include "nyaa/parser.h"
#include "nyaa/ast.h"
#include "nyaa/parser-ctx.h"
#include "nyaa/syntax.hh"
#include "sql/ast-factory.h"
#include "base/arena-utils.h"
#include "base/slice.h"

#define YYLTYPE NYAA_YYLTYPE
#define YYSTYPE NYAA_YYSTYPE
#include "nyaa/lex.yy.h"

namespace mai {
    
namespace nyaa {
    
std::string Parser::Result::ToString() const {
    return base::Sprintf("[%d:%d] %s", error_line, error_column, error->data());
}
    
/*static*/ Parser::Result Parser::Parse(const char *s, int max_trace_id, base::Arena *arena) {
    ast::Factory factory(arena);
    parser_ctx ctx;
    ctx.arena = arena;
    ctx.factory = &factory;
    ctx.next_trace_id = max_trace_id;
    
    lexer_extra extra;
    extra.arena = arena;
    extra.factory = &factory;
    nyaa_yylex_init_extra(&extra, &ctx.lex);
    auto buf = nyaa_yy_scan_string(s, ctx.lex);
    nyaa_yy_switch_to_buffer(buf, ctx.lex);
    nyaa_yyset_lineno(1, ctx.lex);
    nyaa_yyparse(&ctx);
    nyaa_yy_delete_buffer(buf, ctx.lex);
    nyaa_yylex_destroy(ctx.lex);

    return {
        .block = ctx.block,
        .next_trace_id = ctx.next_trace_id,
        .error = ctx.err_msg,
        .error_line = ctx.err_line,
        .error_column = ctx.err_column,
    };
}

/*static*/ Parser::Result Parser::Parse(const char *s, size_t n, int max_trace_id, base::Arena *arena) {
    ast::Factory factory(arena);
    parser_ctx ctx;
    ctx.arena = arena;
    ctx.factory = &factory;
    ctx.next_trace_id = max_trace_id;
    
    lexer_extra extra;
    extra.arena = arena;
    extra.factory = &factory;
    nyaa_yylex_init_extra(&extra, &ctx.lex);
    auto buf = nyaa_yy_scan_bytes(s, n, ctx.lex);
    nyaa_yy_switch_to_buffer(buf, ctx.lex);
    nyaa_yyset_lineno(1, ctx.lex);
    nyaa_yyparse(&ctx);
    nyaa_yy_delete_buffer(buf, ctx.lex);
    nyaa_yylex_destroy(ctx.lex);
    
    return {
        .block = ctx.block,
        .next_trace_id = ctx.next_trace_id,
        .error = ctx.err_msg,
        .error_line = ctx.err_line,
        .error_column = ctx.err_column,
    };
}
    
/*static*/ Parser::Result Parser::Parse(FILE *fp, int max_trace_id, base::Arena *arena) {
    ast::Factory factory(arena);
    parser_ctx ctx;
    ctx.arena = arena;
    ctx.factory = &factory;
    ctx.next_trace_id = max_trace_id;
    
    lexer_extra extra;
    extra.arena = arena;
    extra.factory = &factory;
    nyaa_yylex_init_extra(&extra, &ctx.lex);
    nyaa_yyset_in(fp, ctx.lex);
    //nyaa_yyset_lineno(1, ctx.lex);
    nyaa_yyparse(&ctx);
    nyaa_yylex_destroy(ctx.lex);

    return {
        .block = ctx.block,
        .next_trace_id = ctx.next_trace_id,
        .error = ctx.err_msg,
        .error_line = ctx.err_line,
        .error_column = ctx.err_column,
    };
}
    
} // namespace nyaa

} // namespace mai
