DIGIT   [0-9]
NONZERODIGIT    [1-9]
OCTDIGIT        [0-7]
WHITESPACE      [ \t\v\n\r\f]
ALPHANUMERIC    [a-zA-Z0-9_]
ALPHA   [a-zA-Z_]

%{
#include <string>
#include "parse.hpp"
int comment_nesting = 0;
%}

%x COMMENT
%x LINE_COMMENT

%%

"u1" {return INT1;}
"u8"   {return INT8;}
"u16"   {return INT16;}
"u32"   {return INT32;}
"u64"   {return INT64;}
"u128"   {return INT128;}
"int"   {return INT;}
"string"   {return STRING;}
"return"        {return RETURN;}
"while" {return WHILE;}
"if"    {return IF;}
"else"  {return ELSE;}
":="    {return DEFINE_AND_ASSIGN;}
"=="    {return EQUAL;}
"->"    {return RESULTS_IN;}
"="     {return ASSIGN;}
":"    {return DEFINE;}
"("     {return '(';}
")"     {return ')';}
"{"     {return '{';}
"}"     {return '}';}
"["     {return '[';}
"]"     {return ']';}
","     {return ',';}
";"     {return ';';}
"+"     {return '+';}
"-"     {return '-';}
"*"     {return '*';}
"/"     {return '/';}

{NONZERODIGIT}{DIGIT}*        {yylval.string=new std::string(yytext); return DEC;}
"0"{OCTDIGIT}*  {yylval.string = new std::string(yytext); return OCT;}
{ALPHA}{ALPHANUMERIC}*  {yylval.string = new std::string(yytext); return ID;}

{WHITESPACE}+      {/*Discard */}

"/*"    {BEGIN(COMMENT);}
<COMMENT>{
   "/*" {++comment_nesting;} 
   "*"+"/"      {if(comment_nesting) --comment_nesting;
                 else BEGIN(INITIAL);}
    \n  {}
    .   {}
}

"//"    {BEGIN(LINE_COMMENT);}
<LINE_COMMENT>\n        {BEGIN(INITIAL);}
<LINE_COMMENT>. {}


.       {printf("I didn't understand %s\n", yytext);}
%%

int yywrap(){
    return 1;
}
