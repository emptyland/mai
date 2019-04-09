%option noyywrap reentrant bison-bridge yylineno bison-locations
%option header-file="lex.yy.h"
%option extra-type="::mai::nyaa::lexer_extra *"

%{
// lex --header-file=lex.yy.h lex.l
//struct parser_ctx parser_ctx;

#include "nyaa/parser-ctx.h"
#include "nyaa/ast.h"
#include "nyaa/snytax.hh"
#include "base/slice.h"
#include <string.h>
#include <stdlib.h>

using ::mai::base::Slice;

#define YY_USER_ACTION \
	yylloc->first_line = yylloc->last_line = yylineno; \
	yylloc->first_column = yyextra->column;            \
	yylloc->last_column = (int)(yyextra->column + yyleng - 1);    \
	yyextra->column += (int)yyleng;

#define register
%}
%x COMMENT_MOD

%%
def { return DEF; }
var { return VAR; }
lambda { return LAMBDA; }


[A-Za-z_][A-Za-z_0-9]* {
    //yylval->text.buf = yytext;
    //yylval->text.len = strlen(yytext);
    return NAME;
}

`[A-Za-z_][A-Za-z_0-9]*` {
    //yylval->text.buf = yytext + 1;
    //yylval->text.len = strlen(yytext) - 2;
    return NAME;
}

'(\\.|''|[^'\n'])*' |
\"(\\.|\"\"|[^"\n])*\" {
    //yylval->text.buf = yytext + 1;
    //yylval->text.len = strlen(yytext) - 2;
    return VALUE;	
}

-?[0-9]+ {
	int rv = Slice::ParseI64(yytext, &yylval->i64_val);
	if (rv < 0) {
		return TOKEN_ERROR;
	} else if (rv > 0) {
		yylval->dec_val = Decimal::NewDecLiteral(yytext, strlen(yytext), yyextra->arena);
		return DECIMAL_VAL;
	}
	return VALUE;
}

-?[0-9]+"."[0-9]* |
-?"."[0-9]+ {
	yylval->dec_val = Decimal::NewPointLiteral(yytext, strlen(yytext), yyextra->arena);
	return VALUE;
}

-?[0-9]+E[-+]?[0-9]+ |
-?"."[0-9]+E[-+]?[0-9]+ {
	yylval->dec_val = Decimal::NewExpLiteral(yytext, strlen(yytext), yyextra->arena);
	return VALUE;
}

"==" {
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
"<<" { return OP_LSHIFT; }
">>" { return OP_RSHIFT; }

[-=+&~|^/%!()*.,;{}] { return yytext[0]; }

"--"[ \t].* ;

"/*"          { yyextra->old_state = YY_START; BEGIN COMMENT_MOD; }
<COMMENT_MOD>"*/" { BEGIN yyextra->old_state; }
<COMMENT_MOD>.|\n ;
<COMMENT_MOD><<EOF>> { return TOKEN_ERROR; }

[ \t] ;
[\r\n] { yyextra->column = 1; }

%%

