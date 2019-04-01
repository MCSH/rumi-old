DIGIT   [0-9]
WHITESPACE      [ \t\v\n\r\f]
ALPHANUMERIC    [a-zA-Z0-9_]
ALPHA   [a-zA-Z_]

%{
#include <string>
%}

%%

{ALPHA}{ALPHANUMERIC}*  { printf("I got %s\n", yytext); }

{WHITESPACE}+      {/*Discard */}
%%

int yywrap(){
    return 1;
}
