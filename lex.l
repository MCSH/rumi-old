DIGIT   [0-9]
NONZERODIGIT    [1-9]
OCTDIGIT        [0-7]
WHITESPACE      [ \t\v\n\r\f]
ALPHANUMERIC    [a-zA-Z0-9_]
ALPHA   [a-zA-Z_]

%{
#include <string>
%}

%%

"int"   {printf("Type int\n");}
"string"   {printf("Type string\n");}
":="    {printf("Define and assign\n");}
":"    {printf("Define\n");}
"("     {printf("(\n");}
")"     {printf(")\n");}
"{"     {printf("{\n");}
"}"     {printf("}\n");}
"["     {printf("[\n");}
"]"     {printf("]\n");}
","     {printf(",\n");}
";"     {printf(";\n");}

{NONZERODIGIT}{DIGIT}*        {printf("I got digit %s\n", yytext);}
"0"{OCTDIGIT}*  {printf("I got oct digit %s\n", yytext);}
{ALPHA}{ALPHANUMERIC}*  { printf("I got %s\n", yytext); }

{WHITESPACE}+      {/*Discard */}

.       {printf("I didn't understand %s\n", yytext);}
%%

int yywrap(){
    return 1;
}
