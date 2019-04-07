#include <fstream>
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"

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

    return 0; // TODO

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();

    std::string Error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);

    if(!target){
        llvm::errs() << Error;
        return 1;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto targetMachine = target -> createTargetMachine(targetTriple, CPU, Features, opt, RM);

    cc-> module->setDataLayout(targetMachine->createDataLayout());
    cc-> module->setTargetTriple(targetTriple);



    auto output = "out.o";
    std::error_code EC;
    llvm::raw_fd_ostream dest(output, EC, llvm::sys::fs::F_None);

    if(EC){
        llvm::errs() << "Could not open file: " << EC.message();
        return 1;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::TargetMachine::CGFT_ObjectFile;

    if(targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)){
        llvm::errs() << "Target Machine can't emit a file of this type";
        return 1;
    }

    pass.run(*(cc->module));
    dest.flush();

    delete cc;
    return 0;
}
