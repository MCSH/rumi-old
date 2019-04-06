#pragma once
#include <vector>
#include <cstdio>
#include <llvm/IR/Value.h>

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

class CompileContext{
    public:
        llvm::LLVMContext context;
        llvm::IRBuilder<> *builder;
        std::unique_ptr<llvm::Module> module;
        std::map<std::string, llvm::Value *> namedValues;

        CompileContext(){
            builder = new llvm::IRBuilder<>(context);
        }
};

class Node{
    public:
        virtual ~Node() = default;
        virtual llvm::Value* codegen(CompileContext *cc)=0;
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

class FunctionNode: public StatementNode{
    std::string name;
    public:
    BlockNode *body;
    FunctionNode(std::string name, BlockNode *body) {
        this->name = name;
        this->body = body;
    }
        virtual llvm::Function* codegen(CompileContext *cc);
};

class ExprNode: public StatementNode{
};

class IntNode: public ExprNode{
    public:
        int value;
        IntNode(int value): value(value){}

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