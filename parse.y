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
    TypeNode *type;
}

%token<string> OCT DEC
%token<string> ID
%token<string> INT STRING
%token DEFINE_AND_ASSIGN
%token RESULTS_IN
%token RETURN
%token DEFINE
%token ASSIGN

%type<program> program top_level
%type<statement> top_level_code return_stmt stmt variable_decl variable_assign function_declaration
%type<function> function_definition
%type<expr> value expr function_call
%type<block> code
%type<type> return_type type array_type


%start program
%define parse.error verbose

%%
program
    : top_level {programBlock = $1;};

top_level
    : top_level_code {$$ = new BlockNode; $$->statements.push_back($1);}
    | top_level top_level_code { $1->statements.push_back($2); $$=$1;};

top_level_code
    : function_definition {$$ = $1;}
    | function_declaration{$$=$1;};

function_definition
    : ID DEFINE_AND_ASSIGN '(' params ')' return_type '{' code '}' { $$ = new FunctionNode($8, new FunctionSignature(*$1, $6));};

function_declaration
    : ID DEFINE '(' params ')' return_type ';' {$$=new FunctionSignature(*$1, $6);};

return_type
    : empty {$$=new TypeNode(Types::VOID);}// Void
    | RESULTS_IN type {$$=$2;};

params
    : param_list
    | empty;

param
    : ID DEFINE type;

param_list
    : param ',' param_list
    | param;

type
    : STRING {$$=new TypeNode(Types::STRING);}
    | INT {$$=new TypeNode(Types::INT);}
    | array_type;

array_type
    : '[' ']' type {$$=new TypeNode(Types::STRING);}; // TODO

empty:;

code
    : code  stmt {$1->statements.push_back($2); $$ = $1;}
    | stmt {$$=new BlockNode; $$->statements.push_back($1);};

stmt
    : return_stmt
    | variable_decl
    | variable_assign
    | function_definition {$$=$1;}
    ;

variable_assign
    : ID ASSIGN expr ';' {$$ = new VariableAssignNode(*$1, $3);};

variable_decl
    : ID DEFINE_AND_ASSIGN expr ';' {$$ = new VariableDeclNode(*$1, $3);};

expr
    : value
    | function_call;

function_call
    : ID '(' args ')' {$$=new FunctionCallnode(*$1);};

args
    : arg_list
    | empty;

arg_list
    : arg ',' arg_list
    | arg;

arg: expr;

return_stmt
    : RETURN expr ';' { $$ = new RetNode($2); };

value
    : ID { $$ = new VariableLoadNode(*$1);} // TODO
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
