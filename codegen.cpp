#include "node.h"


llvm::Value* BlockNode::codegen(CompileContext *cc){
    llvm::Value *last = NULL;
    for(auto statement: statements){
        last = statement->codegen(cc);
    }

    return last;
}

llvm::Function* FunctionNode::codegen(CompileContext *cc){

    llvm::Function *f;

    if(declar){
        f = fs->codegen(cc);
    } else {
        f = cc->module->getFunction(fs->name.c_str());
        if(!f){
            llvm::errs() << "Attempt to define function without declaration, have you forgotten a : ?\n At function " << fs->name << "\n";
            exit(1);
            return nullptr;
        }
    }
    

    llvm::BasicBlock *bblock = llvm::BasicBlock::Create(cc->context, "entry", f);
    cc->builder->SetInsertPoint(bblock);

    cc->block.push_back(new BlockContext(bblock));

    body -> codegen(cc);

    cc->block.pop_back();


    llvm::verifyFunction(*f);

    return f;
}

llvm::Function* FunctionSignature::codegen(CompileContext *cc){
    std::vector<llvm::Type*> args;
    auto type = this->type->codegen(cc);

    cc->setType(name, this->type->type);

    llvm::FunctionType *fT = llvm::FunctionType::get(type, args, false);
    return llvm::Function::Create(fT, llvm::Function::ExternalLinkage, name, cc->module.get());
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

        llvm::AllocaInst *alloc = TmpB.CreateAlloca(llvm::Type::getInt64Ty(cc->context), 0, name.c_str()); // TODO Why not cc->builder ? 

        cc->getBlock()->namedValues[name] = alloc;

        // TODO not needed if no assignment

        if(expr){
            VariableAssignNode van(name, expr);
            van.codegen(cc);
            type = expr->type;
        }

        cc->setType(name, this->type->type);

        return alloc;

    }
    return nullptr;
}

llvm::Value* VariableAssignNode::codegen(CompileContext *cc){
    llvm::AllocaInst *alloc = cc->getBlock()->namedValues[name];
    
    if(!alloc){
        llvm::errs() << "Undefined variable " << name;
        return nullptr;
    }

    llvm::Value *v = expr ->codegen(cc);
    return cc->builder->CreateStore(v, alloc); // TODO tmpb or cc->builder?
}

llvm::Value* VariableLoadNode::codegen(CompileContext *cc){
    llvm::Value *v = cc->getBlock()->namedValues[name];

    if(!v){
        llvm::errs() << "Undefined variable " << name << "\n";
        exit(1);
        return nullptr;
    }

    type = new TypeNode(cc->getType(name));

    return cc->builder->CreateLoad(v, "readtmp");
}

llvm::Value* FunctionCallnode::codegen(CompileContext *cc){
    llvm::Function *calleeF = cc->module->getFunction(name.c_str());
    if(!calleeF){
        llvm::errs() << "Undefined function " << name << "\n";
        exit(1);
        return nullptr;
    }

    type = new TypeNode(cc->getType(name));

    std::vector<llvm::Value *> argsV;


    return cc->builder->CreateCall(calleeF, argsV, "calltmp");
}

llvm::Type* TypeNode::codegen(CompileContext *cc){
    switch(type){
        case Types::INT:
            return llvm::Type::getInt64Ty(cc->context);
        case Types::STRING:
            return llvm::Type::getDoubleTy(cc->context); // TODO
        case Types::VOID:
            return llvm::Type::getVoidTy(cc->context);
    }
}

llvm::Value* OpExprNode::codegen(CompileContext *cc){
    llvm::Value *lval = LHS->codegen(cc);
    llvm::Value *rval = RHS->codegen(cc);
    llvm::Instruction::BinaryOps instr;

    switch(op){
        case OP::PLUS:
            instr = llvm::Instruction::Add;
            break;
        case OP::SUB:
            instr = llvm::Instruction::Sub;
            break;
        case OP::MULT:
            instr = llvm::Instruction::Mul;
            break;
        case OP::DIVIDE:
            instr = llvm::Instruction::SDiv;
            break;
    }

    // TODO improve later!
    type = LHS->type;

    return llvm::BinaryOperator::Create(instr, lval, rval, "", cc->getBlock()->bblock);
}
