%option noyywrap reentrant bison-bridge yylineno bison-locations
%option header-file="lex.yy.h"
%option extra-type="::mai::nyaa::lexer_extra *"

%{
// lex --header-file=lex.yy.h lex.l
//struct parser_ctx parser_ctx;

#include "nyaa/parser-ctx.h"
#include "nyaa/ast.h"
#include "base/slice.h"
#include "nyaa/syntax.hh"
#include <string.h>
#include <stdlib.h>

using ::mai::base::Slice;
using ::mai::nyaa::NySmi;
using ::mai::nyaa::Operator;
using namespace ::mai::nyaa::ast;

#define YY_USER_ACTION \
	yylloc->first_line = yylloc->last_line = yylineno; \
	yylloc->first_column = yyextra->column;            \
	yylloc->last_column = (int)(yyextra->column + yyleng - 1);    \
	yyextra->column += (int)yyleng;

#define YYLTYPE NYAA_YYLTYPE
#define YYSTYPE NYAA_YYSTYPE

#define register
%}
%x COMMENT_MOD

%%
def { return DEF; }
var { return VAR; }
lambda { return LAMBDA; }
return { return RETURN; }


[A-Za-z_][A-Za-z_0-9]* {
    yylval->name = yyextra->factory->NewString(yytext, 0);
    return NAME;
}

'(\\.|''|[^'\n'])*' |
\"(\\.|\"\"|[^"\n])*\" {
	yylval->str_val = yyextra->factory->NewString(yytext, yytext[0]);
    return STRING_LITERAL;	
}

-?[0-9]+ {
	int rv = Slice::ParseI64(yytext, &yylval->smi_val);
	if (rv < 0) {
		return TOKEN_ERROR;
	}
	if (rv > 0) {
		yylval->int_val = yyextra->factory->NewString(yytext, 0);
		return INT_LITERAL;
	}
	if (yylval->smi_val < NySmi::kMinValue || yylval->smi_val > NySmi::kMaxValue) {
		yylval->int_val = yyextra->factory->NewString(yytext, 0);
		return INT_LITERAL;
	}
	return SMI_LITERAL;
}

-?[0-9]+"."[0-9]* |
-?"."[0-9]+ {
	yylval->f64_val = atof(yytext);
	return APPROX_LITERAL;
}

-?[0-9]+E[-+]?[0-9]+ |
-?"."[0-9]+E[-+]?[0-9]+ {
	yylval->f64_val = atof(yytext);
	return APPROX_LITERAL;
}

"==" {
	yylval->op = Operator::kEQ;
	return COMPARISON;
}
"!=" |
"<>" {
	yylval->op = Operator::kNE;
	return COMPARISON;	
}
"<=>" {
	yylval->op = Operator::kEQ;
	return COMPARISON;	
}
">" {
	yylval->op = Operator::kGT;
	return COMPARISON;		
}
">=" {
	yylval->op = Operator::kGE;
	return COMPARISON;		
}
"<" {
	yylval->op = Operator::kLT;
	return COMPARISON;		
}
"<=" {
	yylval->op = Operator::kLE;
	return COMPARISON;		
}
"&&" { return OP_AND; }
"||" { return OP_OR; }
"<<" { return OP_LSHIFT; }
">>" { return OP_RSHIFT; }
"[" { return '['; }
"]" { return '['; }

[-=+&~|^/%!()*.,;:{}] { return yytext[0]; }

"--"[ \t].* ;

"/*"          { yyextra->old_state = YY_START; BEGIN COMMENT_MOD; }
<COMMENT_MOD>"*/" { BEGIN yyextra->old_state; }
<COMMENT_MOD>.|\n ;
<COMMENT_MOD><<EOF>> { return TOKEN_ERROR; }

[ \t] ;
[\r\n] { yyextra->column = 1; }

%%

