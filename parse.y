%code requires { #include "node.h" }

%{
#include "node.h"
#include <string>

void yyerror(const char *s);
void yyerror(std::string *s);
extern int yylex();

BlockNode *programBlock;

%}

%union{
    std::string *string;
    BlockNode *program;
    BlockNode *block;
    StatementNode *statement;
    FunctionNode *function;
    ExprNode *expr;
}

%token<string> OCT DEC
%token<string> ID
%token<string> INT STRING
%token DEFINE_AND_ASSIGN
%token RETURN
%token DEFINE

%type<program> program top_level
%type<statement> top_level_code return_stmt
%type<function> function_definition
%type<expr> value
%type<block> code


%start program
%define parse.error verbose

%%
program
    : top_level {programBlock = $1;};

top_level
    : top_level_code {$$ = new BlockNode; $$->statements.push_back($1);}
    | top_level top_level_code { $1->statements.push_back($2); $$=$1;};

top_level_code
    : function_definition {$$ = $1;};

function_definition
    : ID DEFINE_AND_ASSIGN '(' params ')' '{' code '}' { $$ = new FunctionNode(*$1, $7);};

params
    : param_list
    | empty;

param
    : ID DEFINE type;

param_list
    : param ',' param_list
    | param;

type
    : STRING
    | INT
    | array_type;

array_type
    : '[' ']' type;

empty:;

code
    : return_stmt {$$=new BlockNode; $$->statements.push_back($1);};

return_stmt
    : RETURN value ';' { $$ = new RetNode($2); };

value
    : ID { $$ = new IntNode(0);} // TODO
    | OCT {$$ = new IntNode(strtol($1->c_str(), NULL, 8));}
    | DEC { $$ = new IntNode(atoi($1->c_str())); };  // TODO check for long?


%%

void yyerror(const char *s){
    extern int yylineno;	// defined and maintained in lex.c
    extern char *yytext;	// defined and maintained in lex.c
  
    printf("ERROR: %s at symbol %s on line %d\n", s, yytext, yylineno);
}

void yyerror(std::string *s){
    yyerror(s->c_str());
}
