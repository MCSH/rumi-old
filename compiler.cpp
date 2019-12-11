#include "compiler.h"

int yyparse();
extern "C" FILE *yyin;

CompileContext *compile(int argc, char **argv) {

  if (argc = 2) {
    char const *filename = argv[1];
    yyin = fopen(filename, "r");
    assert(yyin);
  }
  extern BlockNode *programBlock;
  yyparse();

  CompileContext *cc = new CompileContext();
  cc->module = llvm::make_unique<llvm::Module>("my cool jit", cc->context);

  programBlock->codegen(cc);

  return cc;
}
