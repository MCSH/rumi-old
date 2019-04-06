int yyparse();
#include "node.h"

int main(int argc, char **argv){
    extern BlockNode *programBlock;
    yyparse();

    CompileContext *cc = new CompileContext();
    cc -> module = llvm::make_unique<llvm::Module>("my cool jit", cc->context);

    programBlock->codegen(cc);

    cc->module->print(llvm::errs(), nullptr);

    delete cc;
    return 0;
}
