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
    auto type = llvm::Type::getInt64Ty(cc->context);

    llvm::FunctionType *fT = llvm::FunctionType::get(type, args, false);
    llvm::Function *f = llvm::Function::Create(fT, llvm::Function::ExternalLinkage, name, cc->module.get());

    llvm::BasicBlock *bblock = llvm::BasicBlock::Create(cc->context, "entry", f);
    cc->builder->SetInsertPoint(bblock);

    cc->block.push_back(new BlockContext(bblock));

    body -> codegen(cc);

    cc->block.pop_back();


    llvm::verifyFunction(*f);

    return f;
}

llvm::Value* IntNode::codegen(CompileContext *cc){
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(cc->context), value, true);
}

llvm::Value* RetNode::codegen(CompileContext *cc){
    return cc->builder->CreateRet(st->codegen(cc));
}

llvm::Value* VariableDeclNode::codegen(CompileContext *cc){
    // TODO check global!
    if(cc->hasBlock()){

        auto bblock = cc->getBlock()->bblock;

        llvm::IRBuilder<> TmpB(bblock, bblock->begin());

        llvm::AllocaInst *alloc = TmpB.CreateAlloca(llvm::Type::getInt64Ty(cc->context), 0, name.c_str());

        cc->getBlock()->namedValues[name.c_str()] = alloc;

        // TODO not needed if no assignment

        llvm::Value *v = expr ->codegen(cc);
        cc->builder->CreateStore(v, alloc); // TODO tmpb or cc->builder?

        return alloc;

    }
    return nullptr;
}
