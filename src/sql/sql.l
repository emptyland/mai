%option noyywrap reentrant bison-bridge yylineno bison-locations
%option header-file="sql.yy.h"

%{
// lex --header-file=sql.yy.h sql.l
//struct parser_ctx parser_ctx;

#include "sql/parser-ctx.h"
#include "sql/ast.h"
#include "sql/sql.hh"
#include <string.h>

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg);

#define register
%}
chars [A-Za-z_$0-9]
digit [0-9]
delim [ \f\r\t\v]

%%
NULL { return NULL_VAL; }
SELECT { return SELECT; }
FROM { return FROM; }
BEGIN { return TXN_BEGIN; }
TRANSACTION { return TRANSACTION; }
COMMIT { return TXN_COMMIT; }
ROLLBACK { return TXN_ROLLBACK; }
SHOW { return SHOW; }
DROP { return DROP; }
TABLE { return TABLE; }
TABLES { return TABLES; }

[A-Za-z_][A-Za-z_0-9]+ {
    yylval->text.buf = yytext;
    yylval->text.len = strlen(yytext);
    return ID;
}

`[A-Za-z_][A-Za-z_0-9]+` {
    yylval->text.buf = yytext + 1;
    yylval->text.len = strlen(yytext) - 2;
    return ID;
}

[0-9]+ {
	return INTEGRAL_VAL;
}

"(" { return '('; }
")" { return ')'; }
"*" { return '*'; }
"." { return '.'; }
"," { return ','; }
";" { return ';'; }

[ \t\n]

%%

