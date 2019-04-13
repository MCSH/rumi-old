#pragma once
#include <vector>
#include <cstdio>
#include <llvm/IR/Value.h>

#include <iostream>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"

enum class PrimTypes{
    INT,
    STRING,
    VOID
};

class Types{
    public:
    PrimTypes type;
    int size;
    bool is_prim;

    Types(){}

    Types(PrimTypes t){
        type = t;
        size = 0;
        is_prim = true;
    }

    Types(PrimTypes t, int s){
        type = t;
        size = s;
        is_prim = false;
    }

    virtual bool compatible(Types *that){
        // if(!that) // TODO I don't like this, but just for now.
            // return false;
        return this->type==that->type;
    }
};

class ArrayTypes: public Types{
    public:
        Types *base;

        ArrayTypes(Types *base){
            this->base = base;
            is_prim = false;
        }

        bool compatible(Types *that){
            ArrayTypes *t = dynamic_cast<ArrayTypes *>(that);
            if(!t)
                return false;
            return base->compatible(t->base);
        };
};


class BlockContext{
    public:
        std::map<std::string, llvm::AllocaInst *> namedValues; // Maybe should be Value*?
        std::map<std::string, Types*> namedTypes;
        llvm::BasicBlock *bblock;

        BlockContext(llvm::BasicBlock *bb){
            this->bblock = bb;
        }
};

class CompileContext{
    public:
        llvm::LLVMContext context;
        llvm::IRBuilder<> *builder;
        std::unique_ptr<llvm::Module> module;
        std::vector<BlockContext *> block;
        BlockContext *global;

        CompileContext(){
            builder = new llvm::IRBuilder<>(context);
            global = new BlockContext(nullptr);
        }

        bool hasBlock(){
            return block.size() != 0;
        }

        BlockContext *getBlock(){
            if(hasBlock())
                return block.back();
            return global;
        }

        Types* getType(std::string name){
            for(std::vector<BlockContext *>::reverse_iterator i=block.rbegin(); i != block.rend(); i++){
                auto nt = (*i)->namedTypes;
                auto p = nt.find(name);
                if(p!=nt.end())
                    return p->second;
            }

            return global->namedTypes[name];
        }

        llvm::AllocaInst* getNamedValue(std::string name){
            // TODO replace usage of namedValues[..] with this func
            for(std::vector<BlockContext *>::reverse_iterator i=block.rbegin(); i != block.rend(); i++){
                auto nt = (*i)->namedValues;
                auto p = nt.find(name);
                if(p!=nt.end())
                    return p->second;
            }

            return global->namedValues[name];
        }

        void setType(std::string name, Types *type){
            if(hasBlock()){
                block.back()->namedTypes[name] = type;
                return;
            }

            global->namedTypes[name] = type;
        }
};

class Node{
    public:
        virtual ~Node() = default;
        virtual llvm::Value* codegen(CompileContext *cc)=0;
};

class TypeNode{
    public:
    Types* type;
    TypeNode(Types *type){
        this->type = type;
    }
    virtual llvm::Type* codegen(CompileContext *cc);
};

class StatementNode: public Node{
    public:
        virtual llvm::Value* codegen(CompileContext *cc)=0;
};

class BlockNode: public StatementNode{
    public:
        std::vector<StatementNode *> statements;
        virtual llvm::Value* codegen(CompileContext *cc);
};

class ExprNode: public StatementNode{
    public:
    virtual llvm::Value* codegen(CompileContext *cc)= 0;
    virtual Types* resolveType(CompileContext *cc) = 0;
};

class IntNode: public ExprNode{
    public:
        int value;
        IntNode(int value): value(value){}

        virtual llvm::Value* codegen(CompileContext *cc);
        virtual Types* resolveType(CompileContext *cc){
            return new Types(PrimTypes::INT);
        }
};

class RetNode: public StatementNode{
    public:
        ExprNode *st;

        RetNode(ExprNode *st){
            this->st = st;
        }

        virtual llvm::Value* codegen(CompileContext *cc);
};

class VariableDeclNode: public StatementNode{
    public:
        std::string name;
        ExprNode *expr;
        TypeNode *type;

        VariableDeclNode(std::string name, ExprNode *expr, TypeNode *type){
            this->name = name;
            this->expr = expr;
            this->type = type;
        }
        virtual llvm::Value* codegen(CompileContext *cc);
};

class VariableAssignNode: public StatementNode{
    public:
        std::string name;
        ExprNode *expr;

        VariableAssignNode(std::string name, ExprNode *expr){
            this->name = name;
            this->expr = expr;
        }
        virtual llvm::Value* codegen(CompileContext *cc);
};

class VariableLoadNode: public ExprNode{
    public:
        std::string name;

        VariableLoadNode(std::string name){
            this->name = name;
        }
        virtual llvm::Value* codegen(CompileContext *cc);
        virtual Types* resolveType(CompileContext *cc);
};

class FunctionCallNode: public ExprNode{
    public:
        std::string name;
        std::vector<ExprNode *> *args;

        FunctionCallNode(std::string name, std::vector<ExprNode *> *args){
            this->name = name;
            this->args = args;
        }
        virtual llvm::Value* codegen(CompileContext *cc);
        virtual Types* resolveType(CompileContext *cc);
};

enum class OP{
    PLUS,
    SUB,
    MULT,
    DIVIDE,
    EQUAL
};

class OpExprNode: public ExprNode{
    public:
        ExprNode *LHS, *RHS;
        OP op;

        OpExprNode(ExprNode *LHS, OP op, ExprNode *RHS){
            this->LHS = LHS;
            this->RHS = RHS;
            this->op = op;
        }
        
        virtual llvm::Value* codegen(CompileContext *cc);
        virtual Types* resolveType(CompileContext *cc);
};

class FunctionSignature: public StatementNode{
    public:
    std::string name;
    TypeNode *type;
    std::vector<VariableDeclNode *> *params;
    FunctionSignature(std::string name, TypeNode *type, std::vector<VariableDeclNode*> *params){
        this->name = name;
        this->type = type;
        this->params = params;
    }

    virtual llvm::Function* codegen(CompileContext *cc);
};

class FunctionNode: public StatementNode{
    public:
    BlockNode *body;
    FunctionSignature *fs;
    bool declar;
    FunctionNode(BlockNode *body, FunctionSignature *fs, bool declar) {
        this->body = body;
        this->fs = fs;
        this->declar = declar;
    }
    virtual llvm::Function* codegen(CompileContext *cc);
};

class WhileNode: public StatementNode{
    public:
    ExprNode *expr;
    StatementNode *block;
    WhileNode(ExprNode *expr, StatementNode *block){
        this->expr = expr;
        this->block = block;
    }

    virtual llvm::Value* codegen(CompileContext *cc);
};

class IfNode: public StatementNode{
    public:
        ExprNode *expr;
        StatementNode *ifblock, *elseblock;

        IfNode(ExprNode *expr, StatementNode *i1, StatementNode *i2){
            this->expr = expr;
            ifblock = i1;
            elseblock = i2;
        }
    virtual llvm::Value* codegen(CompileContext *cc);
};
