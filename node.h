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

enum class PrimTypes{
    INT,
    STRING,
    VOID
};

class Types{
    public:
    PrimTypes type;
    int size;

    Types(PrimTypes t){
        type = t;
        size = 0;
    }

    Types(PrimTypes t, int s){
        type = t;
        size = s;
    }

    bool compatible(Types *that){
        return this->type==that->type;
    }
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

class BlockNode: public Node{
    public:
        std::vector<StatementNode *> statements;
        virtual llvm::Value* codegen(CompileContext *cc);
};

class FunctionSignature: public StatementNode{
    public:
    std::string name;
    TypeNode *type;
    FunctionSignature(std::string name, TypeNode *type){
        this->name = name;
        this->type = type;
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

class FunctionCallnode: public ExprNode{
    public:
        std::string name;
        // TODO args

        FunctionCallnode(std::string name){
            this->name = name;
        }
        virtual llvm::Value* codegen(CompileContext *cc);
        virtual Types* resolveType(CompileContext *cc);
};

enum class OP{
    PLUS,
    SUB,
    MULT,
    DIVIDE
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
