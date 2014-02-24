// -*- mode: c++ -*-
#ifndef _WOMBAT_COMPILER_H
#define _WOMBAT_COMPILER_H

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"

#include "lisp.h"

#include <map>

using namespace llvm;
using namespace std;

class CompileError : public LispException {
public:
    CompileError(string m) : LispException(m) {}
    CompileError(string m, string n) : LispException(m + n) {}
};

class Compiler {

    Module *_mod;
    ExecutionEngine *_exec_eng;
    IRBuilder<> _builder;

    map<Symbol*,Value*> _env;
        
public:

    Compiler();
    
    Function *compile_top_level(Form *f);

    Value *compile(Form *f);
    Value *compile_list(Pair *lis);
    Value *compile_symbol(Symbol *sym);
    Value *compile_quote(Pair *lis);
    Value *compile_def(Pair *lis);
    Value *compile_fn(Pair *lis);

    Value *form_ptr_val(Form *f);
    
    void *get_fn_ptr(Function *f);

    Form *eval(Form *input);
};

#endif
