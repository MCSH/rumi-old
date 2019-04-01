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
%token RETURN
%token DEFINE

%union{
    std::string *string;
}

%start program
%define parse.error verbose

%%
program:
       top_level;

top_level:
         top_level_code
         | top_level top_level_code;

top_level_code:
              function_definition;

function_definition:
                   ID DEFINE_AND_ASSIGN '(' params ')' '{' code '}' {printf("I decoded a function named %s\n", $1->c_str());};

params:
      param_list
      | empty;

param:
     ID DEFINE type;

param_list: 
          param ',' param_list
          | param;

type:
    STRING
    | INT
    | array_type;

array_type:
          '[' ']' type;

empty:;

code:
    RETURN value ';';

value:
     ID
     | OCT
     | DEC;


%%

void yyerror(const char *s){
    extern int yylineno;	// defined and maintained in lex.c
    extern char *yytext;	// defined and maintained in lex.c
  
    printf("ERROR: %s at symbol %s on line %d\n", s, yytext, yylineno);
}

void yyerror(std::string *s){
    yyerror(s->c_str());
}
