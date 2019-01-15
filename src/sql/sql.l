%option noyywrap reentrant bison-bridge yylineno bison-locations
%option header-file="sql.yy.h"
%option extra-type="::mai::sql::lexer_extra *"

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
%x COMMENT_MOD

%%
ADD { return ADD; }
AUTO_INCREMENT { return AUTO_INCREMENT; }
ALTER { return ALTER; }
AFTER { return AFTER; }
AS { return AS; }
ASC { return ASC; }
AND { return OP_AND; }
ALL { return ALL; }
ANY { return ANY; }
BETWEEN { return BETWEEN; }
BY { return BY; }
BEGIN { return TXN_BEGIN; }
BIGINT { return BIGINT; }
COLUMN { return COLUMN; }
CHANGE { return CHANGE; }
COMMIT { return TXN_COMMIT; }
CREATE { return CREATE; }
CROSS { return CROSS; }
COMMENT { return COMMENT; }
CHAR { return CHAR; }
DISTINCT { return DISTINCT; }
DESC { return DESC; }
DROP { return DROP; }
DATE { return DATE; }
DATETIME { return DATETIME; }
DELETE { return DELETE; }
DECIMAL { return DECIMAL; }
DIV { return DIV; }
FROM { return FROM; }
FIRST { return FIRST; }
FOR { return FOR; }
GROUP { return GROUP; }
HAVING { return HAVING; }
INDEX { return INDEX; }
INT { return INT; }
IN { return IN; }
IS { return IS; }
INNER { return INNER; }
INSERT { return INSERT; }
JOIN { return JOIN; }
KEY { return KEY; }
LIKE { return LIKE; }
LEFT { return LEFT; }
LIMIT { return LIMIT; }
MOD { return MOD; }
NUMERIC { return NUMERIC; }
NOT { return NOT; }
NULL { return NULL_VAL; }
OR { return OP_OR; }
ORDER { return ORDER; }
ON { return ON; }
OUTTER { return OUTTER; }
OFFSET { return OFFSET; }
OVERWRITE { return OVERWRITE; }
ROLLBACK { return TXN_ROLLBACK; }
RENAME { return RENAME; }
REGEXP { return REGEXP; }
PRIMARY { return PRIMARY; }
RIGHT { return RIGHT; }
SELECT { return SELECT; }
SHOW { return SHOW; }
SMALLINT { return SMALLINT; }
SET { return SET; }
TRANSACTION { return TRANSACTION; }
TABLE { return TABLE; }
TABLES { return TABLES; }
TO { return TO; }
TINYINT { return TINYINT; }
UPDATE { return UPDATE; }
UNIQUE { return UNIQUE; }
VARCHAR { return VARCHAR; }
VALUES { return VALUES; }
WHERE { return WHERE; }
XOR { return XOR; }



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

-?[0-9]+"."[0-9]* |
-?"."[0-9]+ |
-?[0-9]+E[-+]?[0-9]+ |
-?"."[0-9]+E[-+]?[0-9]+ {
	yylval->approx_val = atof(yytext);
	return APPROX_VAL;
}

"=" {
	yylval->op = ::mai::sql::SQL_CMP_EQ;
	return COMPARISON;
}
"!=" |
"<>" {
	yylval->op = ::mai::sql::SQL_CMP_NE;
	return COMPARISON;	
}
"<=>" {
	yylval->op = ::mai::sql::SQL_CMP_STRICT_EQ;
	return COMPARISON;	
}
">" {
	yylval->op = ::mai::sql::SQL_CMP_GT;
	return COMPARISON;		
}
">=" {
	yylval->op = ::mai::sql::SQL_CMP_GE;
	return COMPARISON;		
}
"<" {
	yylval->op = ::mai::sql::SQL_CMP_LT;
	return COMPARISON;		
}
"<=" {
	yylval->op = ::mai::sql::SQL_CMP_LE;
	return COMPARISON;		
}
"&&" { return OP_AND; }
"||" { return OP_OR; }
"<<" { return LSHIFT; }
">>" { return RSHIFT; }
":=" { return ASSIGN; }

[-+&~|^/%!()*.,;] { return yytext[0]; }

"--"[ \t].* ;

"/*"          { yyextra->old_state = YY_START; BEGIN COMMENT_MOD; }
<COMMENT_MOD>"*/" { BEGIN yyextra->old_state; }
<COMMENT_MOD>.|\n ;
<COMMENT_MOD><<EOF>> { return TOKEN_ERROR; }

[ \t\n] ;

%%

