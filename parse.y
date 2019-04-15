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
    VariableDeclNode *var_decl;
    StructMemberNode *struct_member;
    std::vector<StructMemberNode *> *members;
    std::vector<VariableDeclNode*> *params;
    std::vector<ExprNode *> *args;
}

%token<string> OCT DEC
%token<string> ID SSTRING
%token INT INT1 INT8 INT16 INT32 INT64 INT128
%token STRING
%token DEFINE_AND_ASSIGN
%token RESULTS_IN
%token RETURN
%token DEFINE
%token ASSIGN
%token WHILE
%token IF ELSE
%token TRIPLE_DOT
%token STRUCT

%type<program> program top_level
%type<statement> top_level_code return_stmt stmt variable_decl variable_assign function_declaration while_stmt if_stmt struct_define struct_member_set
%type<function> function_definition
%type<expr> value expr function_call op_expr arg struct_member_access
%type<block> code
%type<type> return_type type array_type

%type<args> args arg_list

%type<var_decl> param
%type<params> params param_list

%type<struct_member> struct_member
%type<members> struct_members

%left EQUAL
%left '+' '-'
%left '*' '/'


// hack to fix if else problem
%nonassoc "then"
%nonassoc ELSE

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
    | function_declaration{$$=$1;}
    | struct_define
    ;

function_definition
    : ID DEFINE_AND_ASSIGN '(' params ')' return_type '{' code '}' { $$ = new FunctionNode($8, new FunctionSignature(*$1, $6, $4), true);}
    | ID ASSIGN '(' params ')' return_type '{' code '}' { $$ = new FunctionNode($8, new FunctionSignature(*$1, $6, $4), false);};

function_declaration
    : ID DEFINE '(' params ')' return_type ';' {$$=new FunctionSignature(*$1, $6, $4);};

return_type
    : empty {$$=new TypeNode(new VoidTypes());}// Void
    | RESULTS_IN type {$$=$2;};

params
    : param_list
    | empty {$$=nullptr;}
    ;

param
    : ID DEFINE type{$$=new VariableDeclNode(*$1, nullptr, $3);}
    | TRIPLE_DOT {$$=new VariableDeclNode("", nullptr, nullptr, true);}
    ;

param_list
    : param_list ',' param {$1->push_back($3); $$=$1;}
    | param {$$=new std::vector<VariableDeclNode *>(); $$->push_back($1);}
    ;

type
    : STRING {$$=new TypeNode(new ArrayTypes( new IntTypes(8) ));} // TODO change int8 to char?
    | INT {$$=new TypeNode(new IntTypes(64));}
    | INT1 {$$=new TypeNode(new IntTypes(1));}
    | INT8 {$$=new TypeNode(new IntTypes(8));}
    | INT16 {$$=new TypeNode(new IntTypes(16));}
    | INT32 {$$=new TypeNode(new IntTypes(32));}
    | INT64 {$$=new TypeNode(new IntTypes(64));}
    | INT128 {$$=new TypeNode(new IntTypes(128));}
    | array_type
    | ID {$$=new TypeNode(new StructType(*$1));}
    ;

array_type
    : '[' ']' type {$$=new TypeNode(new ArrayTypes($3->type));};

empty:;

code
    : code  stmt {$1->statements.push_back($2); $$ = $1;}
    | stmt {$$=new BlockNode; $$->statements.push_back($1);};

stmt
    : return_stmt
    | variable_decl
    | variable_assign
    | struct_member_set
    | function_definition {$$=$1;}
    | while_stmt
    | if_stmt
    | function_call ';' {$$=$1;}
    | '{' code '}' {$$=$2;}
    | struct_define
    ;

while_stmt
    : WHILE expr stmt {$$=new WhileNode($2, $3);}
    ;

if_stmt
    : IF expr stmt %prec "then" {$$=new IfNode($2, $3, nullptr);}
    | IF expr stmt ELSE stmt {$$=new IfNode($2, $3, $5);}
    ;

variable_assign
    : ID ASSIGN expr ';' {$$ = new VariableAssignNode(*$1, $3);};

variable_decl
    : ID DEFINE_AND_ASSIGN expr ';' {$$ = new VariableDeclNode(*$1, $3, nullptr);}
    | ID DEFINE type ASSIGN expr ';' {$$=new VariableDeclNode(*$1, $5, $3);}
    | ID DEFINE type ';' {$$=new VariableDeclNode(*$1, nullptr, $3);}
    ;

expr
    : function_call
    | op_expr
    | value
    | '(' expr ')' {$$=$2;}
    | struct_member_access /* TODO Shift reduce problem is here probably */
    ;

struct_member_access
    : expr '.' ID {$$=new StructMemberAccessNode($1, *$3);}
    ;

struct_member_set
    : expr '.' ID ASSIGN expr ';' {$$=new StructMemberSetNode($1, *$3, $5);}
    ;

op_expr
    : expr '+' expr{$$=new OpExprNode($1,OP::PLUS,$3);}
    | expr '-' expr{$$=new OpExprNode($1,OP::SUB,$3);}
    | expr '*' expr{$$=new OpExprNode($1,OP::MULT,$3);}
    | expr '/' expr{$$=new OpExprNode($1,OP::DIVIDE,$3);}
    | expr EQUAL expr{$$=new OpExprNode($1, OP::DIVIDE,$3);}
    ;

function_call
    : ID '(' args ')' {$$=new FunctionCallNode(*$1, $3);};

args
    : arg_list
    | empty {$$=nullptr;};

arg_list
    : arg_list ',' arg {$1->push_back($3); $$=$1;}
    | arg {$$=new std::vector<ExprNode *>; $$->push_back($1);};

arg: expr;

return_stmt
    : RETURN expr ';' { $$ = new RetNode($2); }
    | RETURN ';' {$$ = new RetNode(nullptr);}
    ;

value
    : ID { $$ = new VariableLoadNode(*$1);} // TODO
    | OCT {$$ = new IntNode(strtol($1->c_str(), NULL, 8));}
    | DEC { $$ = new IntNode(atoi($1->c_str())); }  // TODO check for long?
    | SSTRING {$$=new SStringNode($1->c_str());}
    ;

struct_define
    : STRUCT ID '{' struct_members '}' {$$=new StructNode(*$2, $4);}
    ;

struct_members
    : struct_member struct_members {$$=$2; $$->push_back($1);}
    | struct_member {$$=new std::vector<StructMemberNode *>;$$->push_back($1);}
    ;

struct_member
    : ID DEFINE type ';' {$$=new StructMemberNode(*$1, $3);}
    ;


%%

void yyerror(const char *s){
    extern int yylineno;	// defined and maintained in lex.c
    extern char *yytext;	// defined and maintained in lex.c
  
    printf("ERROR: %s at symbol %s on line %d\n", s, yytext, yylineno);
}

void yyerror(std::string *s){
    yyerror(s->c_str());
}
