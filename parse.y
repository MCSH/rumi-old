%{
#include <string>

void yyerror(const char *s);
void yyerror(std::string *s);
extern int yylex();
%}

%token<string> OCT DEC
%token<string> ID
%token<string> INT STRING
%token DEFINE_AND_ASSIGN
%token DEFINE

%union{
    std::string *string;
}

%%
program:
       ID {printf("I got an Id with name %s\n", $1->c_str());};

%%

void yyerror(const char *s){
    extern int yylineno;	// defined and maintained in lex.c
    extern char *yytext;	// defined and maintained in lex.c
  
    printf("ERROR: %s at symbol %s on line %d\n", s, yytext, yylineno);
}

void yyerror(std::string *s){
    yyerror(s->c_str());
}
