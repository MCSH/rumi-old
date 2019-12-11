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
    if(fs->params)
    for(auto &arg: *fs->params){
        cc->setType(arg->name, arg->type->type);
        llvm::AllocaInst *alloc = TmpB.CreateAlloca(arg->type->codegen(cc), 0, arg->name);
        cc->getBlock()->namedValues[arg->name] = alloc;
    }

    for(auto &Arg: f->args()){
        cc->builder->CreateStore(&Arg, cc->getBlock()->namedValues[Arg.getName()]);
    }

    body -> codegen(cc);

    auto myType = fs->type->type;
    bool is_void = myType->ptype==PrimTypes::VOID;
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

    llvm::FunctionType *fT = llvm::FunctionType::get(type, args, has_vararg);

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
    if(is_vararg)
        return nullptr;
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
                type = new TypeNode(tmp); // TODO: problematic.
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
    llvm::AllocaInst *alloc = cc->getNamedValue(name);
    
    if(!alloc){
        llvm::errs() << "Undefined variable " << name;
        return nullptr;
    }

    llvm::Value *v = expr ->codegen(cc);
    return cc->builder->CreateStore(v, alloc);
}

llvm::Value* VariableLoadNode::codegen(CompileContext *cc){
    llvm::Value *v = cc->getNamedValue(name);

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

    if(cc->getType(name.c_str())->ptype==PrimTypes::VOID)
        return cc->builder->CreateCall(calleeF, argsV, "");
    return cc->builder->CreateCall(calleeF, argsV, "calltmp");
}

Types *FunctionCallNode::resolveType(CompileContext *cc){
    // TODO args!
    return cc->getType(name);
}

llvm::Type* TypeNode::codegen(CompileContext *cc){
    ArrayTypes *ar;
    if(ar = dynamic_cast<ArrayTypes *>(type)){
        return llvm::PointerType::getUnqual( // TODO?
            (new TypeNode(ar->base))->codegen(cc)
                );
    }

    if(type->ptype == PrimTypes::INT){
        IntTypes *t = (IntTypes*)type;
        if(!t->size) 
            // TODO return c int, for now 64
            return llvm::Type::getInt64Ty(cc->context);
        switch(t->size){
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
    } else if(type->ptype == PrimTypes::VOID)
            return llvm::Type::getVoidTy(cc->context);
    else if(type->ptype == PrimTypes::STRUCT){
        StructType *t = (StructType*)type;
        return cc->getStructType(t->name);
        // TODO oh boy
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
        case OP::EQUAL:
            return cc->builder->CreateICmpEQ(lval, rval, "iseq");
    }

    return llvm::BinaryOperator::Create(instr, lval, rval, "", cc->getBlock()->bblock);
}

Types *OpExprNode::resolveType(CompileContext *cc){
    // TODO improve later!
    return LHS->resolveType(cc);
}

llvm::Value* WhileNode::codegen(CompileContext *cc){
    // TODO implement
    // TODO context management?!

    llvm::Function *f = cc->builder->GetInsertBlock()->getParent();

    llvm::BasicBlock 
        * whileCondBB = llvm::BasicBlock::Create(cc->context, "whilecond", f),
        *whileBB = llvm::BasicBlock::Create(cc->context, "while"),
        *whileEndBB = llvm::BasicBlock::Create(cc->context, "whilecont");

    // Condition

    cc->block.push_back(new BlockContext(whileCondBB));

    cc->builder->CreateBr(whileCondBB);
    cc->builder->SetInsertPoint(whileCondBB);
    llvm::Value *cond = expr->codegen(cc);
    cond = cc->builder->CreateICmpNE(cond, llvm::ConstantInt::get(llvm::Type::getInt64Ty(cc->context), 0, false), "whilecond");
    cc->builder->CreateCondBr(cond, whileBB, whileEndBB);

    // While Body
    cc->block.push_back(new BlockContext(whileBB));
    f->getBasicBlockList().push_back(whileBB);
    cc->builder->SetInsertPoint(whileBB);
    block->codegen(cc);
    cc->builder->CreateBr(whileCondBB);

    // Out of while
    cc->block.pop_back();
    cc->block.pop_back();
    f->getBasicBlockList().push_back(whileEndBB);
    cc->builder->SetInsertPoint(whileEndBB);

    return nullptr;
}

llvm::Value* IfNode::codegen(CompileContext *cc){
    bool has_else = elseblock != nullptr;


    llvm::Function *f = cc->builder->GetInsertBlock()->getParent();

    llvm::BasicBlock
        *ifBB = llvm::BasicBlock::Create(cc->context, "if", f),
        *elseBB,
        *mergeBB = llvm::BasicBlock::Create(cc->context, "ifcont");

    if(has_else) 
        elseBB = llvm::BasicBlock::Create(cc->context, "else");
    else
        elseBB = mergeBB;

    llvm::Value *cond = expr->codegen(cc);
    cond = cc->builder->CreateICmpNE(cond, llvm::ConstantInt::get(llvm::Type::getInt64Ty(cc->context), 0, false), "ifcond");
    cc->builder->CreateCondBr(cond, ifBB, elseBB);

    // IF Body
    cc->block.push_back(new BlockContext(ifBB));
    cc->builder->SetInsertPoint(ifBB);
    ifblock->codegen(cc);
    cc->builder->CreateBr(mergeBB);
    cc->block.pop_back();

    // Else Body
    if(has_else){
        cc->block.push_back(new BlockContext(elseBB));
        f->getBasicBlockList().push_back(elseBB);
        cc->builder->SetInsertPoint(elseBB);
        elseblock->codegen(cc);
        cc->builder->CreateBr(mergeBB);
        cc->block.pop_back();
    }

    f->getBasicBlockList().push_back(mergeBB);
    cc->builder->SetInsertPoint(mergeBB);

    return nullptr;
}

Types* SStringNode::resolveType(CompileContext *cc){
    return new ArrayTypes(new IntTypes(8)); // TODO change int8 to char?
}

llvm::Value* SStringNode::codegen(CompileContext *cc){
    // TODO
    // return llvm::ConstantDataArray::getString(cc->context, val, true);
    return cc->builder->CreateGlobalStringPtr(val);
}

llvm::Value* StructNode::codegen(CompileContext *cc){
    std::vector<llvm::Type *> membrs_t;
    if(members)
        for(auto m: *members){
            membrs_t.push_back(m->type->codegen(cc));
        }
    auto code = llvm::StructType::create(cc->context, membrs_t, name, false);
    cc->setStructType(name, code);
    cc->setStruct(name, this);
    return nullptr;
}

Types* StructNode::resolveType(CompileContext *cc){
    return new StructType(name); // TODO I think there is no need for any type here.
}

llvm::Value* StructMemberNode::codegen(CompileContext *cc){
    return (llvm::Value*)type->codegen(cc);
}

Types* StructMemberNode::resolveType(CompileContext *cc){
    return type->type;
}

llvm::Value* StructMember::getPtr(CompileContext *cc){
    Types *st = expr->resolveType(cc);
    if(st->ptype != PrimTypes::STRUCT){
        llvm::errs()<<"Can not access value of non struct expressions\n";
        exit(1);
        return nullptr;
    }
    StructType *t = (StructType *) st;
    auto s = cc->getStruct(t->name);
    // TODO  error handling.

    int idx = -1; // TODO
    int i = 0;
    for(auto mem: *s->members){
        llvm::errs()<<"Debug, name is " << mem->name << "\n";
        if(mem->name==member){
            idx = i;
            llvm::errs()<<"I'm here\n";
            break;
        }
        i++;
    }

    if(idx == -1){
        llvm::errs()<< "Struct has no member " << member << "\n";
        exit(1);
        return nullptr;
    }

    llvm::errs()<<"Debug, idx is " << idx << "\n";


    llvm::Value* member_index = llvm::ConstantInt::get(cc->context, llvm::APInt(32, idx, true));

    std::vector<llvm::Value*> indices(2);
    indices[0] = llvm::ConstantInt::get(cc->context, llvm::APInt(32, 0, true));
    indices[1] = member_index;

    auto data = expr->codegen(cc);
    auto stype = cc->getStructType(t->name);

    // Why? TODO
    llvm::AllocaInst* alloc = cc->builder->CreateAlloca(stype, 0, "alloctmp");
    cc->builder->CreateStore(data, alloc);


    auto ptr = llvm::PointerType::getUnqual(stype);

    return llvm::GetElementPtrInst::Create(stype, alloc, indices, "", cc->getBlock()->bblock);
}

Types* StructMember::resolveType(CompileContext *cc){
    Types *st = expr->resolveType(cc);
    if(st->ptype != PrimTypes::STRUCT){
        llvm::errs()<<"Can not access value of non struct expressions\n";
        exit(1);
        return nullptr;
    }
    StructType *t = (StructType *) st;
    auto s = cc->getStruct(t->name);
    // TODO  error handling.

    for(auto mem: *s->members){
        if(mem->name==member)
            return mem->resolveType(cc);
    }

    // TODO  error handling.
}

llvm::Value* StructMemberAccessNode::codegen(CompileContext *cc){
    auto mem_ptr = mem->getPtr(cc);
    return cc->builder->CreateLoad(mem_ptr, "loadtmp");
}

Types* StructMemberAccessNode::resolveType(CompileContext *cc){
    return mem->resolveType(cc);
}

llvm::Value* StructMemberSetNode::codegen(CompileContext *cc){
    auto mem_ptr = mem->getPtr(cc);
    auto v = expr->codegen(cc);

    return cc->builder->CreateStore(v, mem_ptr);
}
