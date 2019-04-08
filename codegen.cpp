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

    llvm::IRBuilder<> TmpB(bblock, bblock->begin());

    // TODO optimize
    auto nv = cc->getBlock()->namedValues;
    if(fs->params)
    for(auto &arg: *fs->params){
        printf("Defining variable %s\n", arg->name.c_str());
        cc->setType(arg->name, arg->type->type);
        llvm::AllocaInst *alloc = TmpB.CreateAlloca(arg->type->codegen(cc), 0, arg->name);
        nv[arg->name] = alloc;
    }

    for(auto &Arg: f->args()){
        cc->builder->CreateStore(&Arg, nv[Arg.getName()]);
    }

    body -> codegen(cc);

    auto myType = fs->type->type;
    bool is_void = myType->type==PrimTypes::VOID;
    RetNode *p = NULL;
    llvm::Value *last = NULL;
    for(auto statement: body->statements){
        if(p = dynamic_cast<RetNode *>(statement)){
            // Check to see if their type matches.

            if(is_void){
                if(p->st){
                    llvm::errs() << "uncompatible return type in function " << fs->name <<"\n";
                    exit(1);
                    return nullptr;
                }
            } else {

                if(!p->st){
                    llvm::errs() << "uncompatible return type in function " << fs->name <<"\n";
                    exit(1);
                    return nullptr;
                }

                auto retType = p->st->resolveType(cc);

                if(!myType->compatible(retType)){
                    llvm::errs() << "Uncompatible return type in function " << fs->name <<"\n";
                    exit(1);
                    return nullptr;
                } 
            }
        }
    }

    cc->block.pop_back();

    llvm::verifyFunction(*f);

    return f;
}

llvm::Function* FunctionSignature::codegen(CompileContext *cc){
    std::vector<llvm::Type*> args;
    if(params)
        for(auto arg: *params){
            args.push_back(arg->type->codegen(cc));
        }
    auto type = this->type->codegen(cc);

    cc->setType(name, this->type->type);

    llvm::FunctionType *fT = llvm::FunctionType::get(type, args, false);

    auto F = llvm::Function::Create(fT, llvm::Function::ExternalLinkage, name, cc->module.get());

    unsigned idx = 0;
    for(auto &arg: F->args())
        arg.setName((*params)[idx++]->name);

    return F;
}

llvm::Value* IntNode::codegen(CompileContext *cc){
    // TODO is it 64?
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(cc->context), value, true);
}

llvm::Value* RetNode::codegen(CompileContext *cc){
    if(st){
        return cc->builder->CreateRet(st->codegen(cc));
    }

    return cc->builder->CreateRetVoid();
}

llvm::Value* VariableDeclNode::codegen(CompileContext *cc){
    // TODO check global!
    if(cc->hasBlock()){
        if(expr){
            auto tmp = expr->resolveType(cc);

            if(this->type){
                // We have a type, and an expression.
                if(!(this->type->type->compatible(tmp))){
                    llvm::errs() << "Assigned expr not compatible with assigned type. variable name: " << name << "\n";
                    exit(1);
                    return nullptr;
                }
            } else {
                // We don't have a type, so our type is the expr assigned to us.
                type = new TypeNode(tmp);
            }
        } else {
            if(!this->type){
                llvm::errs() << "We need either a type or an expr. variable name: " << name << "\n";
                exit(1);
                return nullptr;
            }
        }

        auto bblock = cc->getBlock()->bblock;
        llvm::IRBuilder<> TmpB(bblock, bblock->begin());

        auto t = this->type->codegen(cc);
        llvm::AllocaInst *alloc = TmpB.CreateAlloca(t, 0, name.c_str());

        cc->getBlock()->namedValues[name] = alloc;
        cc->setType(name, this->type->type);

        if(expr){
            VariableAssignNode van(name, expr);
            van.codegen(cc);
        }

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
    return cc->builder->CreateStore(v, alloc);
}

llvm::Value* VariableLoadNode::codegen(CompileContext *cc){
    llvm::Value *v = cc->getBlock()->namedValues[name];

    if(!v){
        llvm::errs() << "Undefined variable " << name << "\n";
        exit(1);
        return nullptr;
    }

    return cc->builder->CreateLoad(v, "readtmp");
}

Types *VariableLoadNode::resolveType(CompileContext *cc){
    return cc->getType(name);
}

llvm::Value* FunctionCallNode::codegen(CompileContext *cc){
    llvm::Function *calleeF = cc->module->getFunction(name.c_str());
    if(!calleeF){
        llvm::errs() << "Undefined function " << name << "\n";
        exit(1);
        return nullptr;
    }

    std::vector<llvm::Value *> argsV;
    if(args)
        for(auto arg: *args){
            argsV.push_back(arg->codegen(cc));
        }


    return cc->builder->CreateCall(calleeF, argsV, "calltmp");
}

Types *FunctionCallNode::resolveType(CompileContext *cc){
    // TODO args!
    return cc->getType(name);
}

llvm::Type* TypeNode::codegen(CompileContext *cc){
    switch(type->type){
        case PrimTypes::INT:
            if(!type->size) 
                // TODO return c int, for now 64
                return llvm::Type::getInt64Ty(cc->context);
            switch(type->size){
                case 1:
                    return llvm::Type::getInt1Ty(cc->context);
                case 8:
                    return llvm::Type::getInt8Ty(cc->context);
                case 16:
                    return llvm::Type::getInt16Ty(cc->context);
                case 32:
                    return llvm::Type::getInt32Ty(cc->context);
                case 64:
                    return llvm::Type::getInt64Ty(cc->context);
                case 128:
                    return llvm::Type::getInt128Ty(cc->context);
            }
        case PrimTypes::STRING:
            return llvm::Type::getDoubleTy(cc->context); // TODO
        case PrimTypes::VOID:
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

    return llvm::BinaryOperator::Create(instr, lval, rval, "", cc->getBlock()->bblock);
}

Types *OpExprNode::resolveType(CompileContext *cc){
    // TODO improve later!
    return LHS->resolveType(cc);
}
