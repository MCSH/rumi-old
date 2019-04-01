DIGIT   [0-9]
NONZERODIGIT    [1-9]
OCTDIGIT        [0-7]
WHITESPACE      [ \t\v\n\r\f]
ALPHANUMERIC    [a-zA-Z0-9_]
ALPHA   [a-zA-Z_]

%{
#include <string>
#include "parse.hpp"
%}

%%

"int"   {return INT;}
"string"   {return STRING;}
"return"        {return RETURN;}
":="    {return DEFINE_AND_ASSIGN;}
":"    {return DEFINE;}
"("     {return '(';}
")"     {return ')';}
"{"     {return '{';}
"}"     {return '}';}
"["     {return '[';}
"]"     {return ']';}
","     {return ',';}
";"     {return ';';}

{NONZERODIGIT}{DIGIT}*        {yylval.string=new std::string(yytext); return DEC;}
"0"{OCTDIGIT}*  {yylval.string = new std::string(yytext); return OCT;}
{ALPHA}{ALPHANUMERIC}*  {yylval.string = new std::string(yytext); return ID;}

{WHITESPACE}+      {/*Discard */}

.       {printf("I didn't understand %s\n", yytext);}
%%

int yywrap(){
    return 1;
}
