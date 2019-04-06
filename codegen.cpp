#include "node.h"


llvm::Value* BlockNode::codegen(CompileContext *cc){
    llvm::Value *last = NULL;
    for(auto statement: statements){
        last = statement->codegen(cc);
    }

    return last;
}

llvm::Function* FunctionNode::codegen(CompileContext *cc){
    std::vector<llvm::Type*> args;
    auto type = llvm::Type::getDoubleTy(cc->context);

    llvm::FunctionType *fT = llvm::FunctionType::get(type, args, false);
    llvm::Function *f = llvm::Function::Create(fT, llvm::Function::ExternalLinkage, name, cc->module.get());

    llvm::BasicBlock *bblock = llvm::BasicBlock::Create(cc->context, "entry", f);
    cc->builder->SetInsertPoint(bblock);

    body -> codegen(cc);


    llvm::verifyFunction(*f);

    return f;
}

llvm::Value* IntNode::codegen(CompileContext *cc){
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(cc->context), value, true);
}

llvm::Value* RetNode::codegen(CompileContext *cc){
    return cc->builder->CreateRet(st->codegen(cc));
}
