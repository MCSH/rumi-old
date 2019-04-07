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

enum class Types{
    INT,
    STRING,
    VOID
};

class BlockContext{
    public:
        std::map<std::string, llvm::AllocaInst *> namedValues; // Maybe should be Value*?
        std::map<std::string, Types> namedTypes;
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

        Types getType(std::string name){
            for(std::vector<BlockContext *>::reverse_iterator i=block.rbegin(); i != block.rend(); i++){
                auto nt = (*i)->namedTypes;
                auto p = nt.find(name);
                if(p!=nt.end())
                    return p->second;
            }

            return global->namedTypes[name];
        }
};

class Node{
    public:
        virtual ~Node() = default;
        virtual llvm::Value* codegen(CompileContext *cc)=0;
};

class TypeNode{
    public:
    Types type;
    TypeNode(Types type){
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
    std::string name;
    public:
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
    FunctionNode(BlockNode *body, FunctionSignature *fs) {
        this->body = body;
        this->fs = fs;
    }
    virtual llvm::Function* codegen(CompileContext *cc);
};

class ExprNode: public StatementNode{
    public:
    TypeNode *type;
    virtual llvm::Value* codegen(CompileContext *cc)= 0;
};

class IntNode: public ExprNode{
    public:
        int value;
        IntNode(int value): value(value){
            type = new TypeNode(Types::INT);
        }

        virtual llvm::Value* codegen(CompileContext *cc);
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
            // TODO fill in type
        }
        virtual llvm::Value* codegen(CompileContext *cc);
};

class FunctionCallnode: public ExprNode{
    public:
        std::string name;
        // TODO args

        FunctionCallnode(std::string name){
            this->name = name;
            // TODO fill in type
        }
        virtual llvm::Value* codegen(CompileContext *cc);
};
