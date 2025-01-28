%require "3.8.2"
%language "C++"
%header "tjspp.tab.hpp"
%output "tjspp.tab.cpp"

%define api.namespace {TJSPP}
%parse-param { tTJSPPExprParser *ptr }
%lex-param   { tTJSPPExprParser *ptr }

%code top {
#include <malloc.h>

#include "tjsTypes.h"
#include "tjs.h"
#include "tjsCompileControl.h"

#define YYMALLOC	::malloc
#define YYREALLOC	::realloc
#define YYFREE		::free

}

%code requires {

namespace TJSPP {
    class tTJSPPExprParser;
}
}

%union
{
	tjs_int32 val;
	tjs_int nv;
}


%token
	PT_LPARENTHESIS				"("
	PT_RPARENTHESIS				")"
	PT_ERROR

	PT_COMMA					","
	PT_EQUAL					"="
	PT_NOTEQUAL					"!="
	PT_EQUALEQUAL				"=="
	PT_LOGICALOR				"||"
	PT_LOGICALAND				"&&"
	PT_VERTLINE					"|"
	PT_CHEVRON					"^"
	PT_AMPERSAND				"&"
	PT_LT						"<"
	PT_GT						">"
	PT_LTOREQUAL				"<="
	PT_GTOREQUAL				">="
	PT_PLUS						"+"
	PT_MINUS					"-"
	PT_ASTERISK					"*"
	PT_SLASH					"/"
	PT_PERCENT					"%"
	PT_EXCLAMATION				"!"

	PT_UN

%token <nv>		PT_SYMBOL
%token <val>	PT_NUM

%type <val>		expr


%left	","
%left	"||"
%left	"&&"
%left	"|"
%left	"^"
%left	"&"
%left   "="
%left	"!=" "=="
%left	"<" ">" "<=" ">="
%left	"+" "-"
%left	"%" "/" "*"
%right	"!" PT_UN

%%

input
	: expr						{ ptr->Result = $1; }
;

expr
	: expr "," expr				{ $$ = $3; }
	| PT_SYMBOL "=" expr		{ ptr->GetTJS()->SetPPValue(ptr->GetString($1), $3); $$ = $3; }
	| expr "!="	expr			{ $$ = $1 != $3; }
	| expr "==" expr			{ $$ = $1 == $3; }
	| expr "||" expr			{ $$ = $1 || $3; }
	| expr "&&" expr			{ $$ = $1 && $3; }
	| expr "|" expr				{ $$ = $1 | $3; }
	| expr "^" expr				{ $$ = $1 ^ $3; }
	| expr "&" expr				{ $$ = $1 & $3; }
	| expr "<" expr				{ $$ = $1 < $3; }
	| expr ">" expr				{ $$ = $1 > $3; }
	| expr ">=" expr			{ $$ = $1 >= $3; }
	| expr "<=" expr			{ $$ = $1 <= $3; }
	| expr "+" expr				{ $$ = $1 + $3; }
	| expr "-" expr				{ $$ = $1 - $3; }
	| expr "%" expr				{ $$ = $1 % $3; }
    | expr "*" expr				{ $$ = $1 * $3; }
	| expr "/" expr				{ if($3==0) { YYABORT; } else { $$ = $1 / $3; } }
	| "!" expr					{ $$ = ! $2; }
	| "+" expr %prec PT_UN		{ $$ = + $2; }
    | "-" expr %prec PT_UN		{ $$ = - $2; }
	| "(" expr ")"				{ $$ = $2; }
	| PT_NUM					{ $$ = $1; }
	| PT_SYMBOL					{ $$ = ptr->GetTJS()->GetPPValue(ptr->GetString($1)); }
;

%%