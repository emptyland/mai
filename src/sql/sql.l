%option noyywrap reentrant bison-bridge yylineno bison-locations
%option header-file="sql.yy.h"

%{
// lex --header-file=sql.yy.h sql.l
//struct parser_ctx parser_ctx;

#include "sql/parser-ctx.h"
#include "sql/ast.h"
#include "sql/sql.hh"
#include <string.h>
#include <stdlib.h>

void yyerror(YYLTYPE *yyl, parser_ctx *ctx, const char *msg);

#define register
%}
chars [A-Za-z_$0-9]
digit [0-9]
delim [ \f\r\t\v]

%%
ADD { return ADD; }
AUTO_INCREMENT { return AUTO_INCREMENT; }
ALTER { return ALTER; }
AFTER { return AFTER; }
AS { return AS; }
CHANGE { return CHANGE; }
FIRST { return FIRST; }
RENAME { return RENAME; }
COLUMN { return COLUMN; }
NULL { return NULL_VAL; }
SELECT { return SELECT; }
FROM { return FROM; }
BEGIN { return TXN_BEGIN; }
TRANSACTION { return TRANSACTION; }
COMMIT { return TXN_COMMIT; }
ROLLBACK { return TXN_ROLLBACK; }
SHOW { return SHOW; }
DROP { return DROP; }
CREATE { return CREATE; }
TABLE { return TABLE; }
TABLES { return TABLES; }
TO { return TO; }
BIGINT { return BIGINT; }
INT { return INT; }
SMALLINT { return SMALLINT; }
TINYINT { return TINYINT; }
DECIMAL { return DECIMAL; }
NUMERIC { return NUMERIC; }
CHAR { return CHAR; }
VARCHAR { return VARCHAR; }
DATE { return DATE; }
DATETIME { return DATETIME; }
PRIMARY { return PRIMARY; }
INDEX { return INDEX; }
KEY { return KEY; }
UNIQUE { return UNIQUE; }
COMMENT { return COMMENT; }

[A-Za-z_][A-Za-z_0-9]* {
    yylval->text.buf = yytext;
    yylval->text.len = strlen(yytext);
    return ID;
}

`[A-Za-z_][A-Za-z_0-9]*` {
    yylval->text.buf = yytext + 1;
    yylval->text.len = strlen(yytext) - 2;
    return ID;
}

'(\\.|''|[^'\n'])*' |
\"(\\.|\"\"|[^"\n])*\" {
    yylval->text.buf = yytext + 1;
    yylval->text.len = strlen(yytext) - 2;
    return STRING_VAL;	
}

-?[0-9]+ {
	yylval->int_val = atoi(yytext);
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

