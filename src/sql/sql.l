%option noyywrap reentrant bison-bridge yylineno bison-locations
%option header-file="sql.yy.h"

%{
// lex --header-file=sql.yy.h sql.l
typedef struct parser_ctx parser_ctx;

#include "sql/sql.hh"
#include <string.h>

#define register
%}
chars [A-Za-z_$0-9]
digit [0-9]
delim [ \f\r\t\v]

%%
SELECT return TOK_SELECT;
FROM return TOK_FROM;

`[A-Za-z_][A-Za-z_0-9]+` {
    yylval->text.buf = yytext + 1;
    yylval->text.len = strlen(yytext) - 2;
}

"(" { return '('; }
")" { return ')'; }
"*" { return '*'; }

%%

