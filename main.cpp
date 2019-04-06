#include <fstream>

int yyparse();
extern "C" FILE *yyin;
#include "node.h"

int main(int argc, char **argv){
    if(argc = 2){
        char const *filename = argv[1];
        yyin = fopen(filename, "r");
        assert(yyin);
    }
    extern BlockNode *programBlock;
    yyparse();

    CompileContext *cc = new CompileContext();
    cc -> module = llvm::make_unique<llvm::Module>("my cool jit", cc->context);

    programBlock->codegen(cc);

    cc->module->print(llvm::outs(), nullptr);

    delete cc;
    return 0;
}
