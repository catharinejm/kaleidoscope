#include "compiler.h"

#include <memory>

#include "llvm/IR/TypeBuilder.h"

// namespace llvm {
//     template<bool xcompile> class TypeBuilder<Form*, xcompile> {
//     public:
//         static const PointerType * get(LLVMContext &Context) {
//             // If you cache this result, be sure to cache it separately
//             // for each LLVMContext.
//             return PointerType::getUnqual(0);
//         }

//         // You may find this a convenient place to put some constants
//         // to help with getelementptr.  They don't have any effect on
//         // the operation of TypeBuilder.
//         // enum Fields {
//         //     FIELD_A,
//         //     FIELD_B,
//         //     FIELD_ARRAY
//         // };
//     };
// }  // namespace llvm

class ExprCompiler : public gc_cleanup {
};

class PrototypeAST : public ExprCompiler {
    string Name;
    vector<string> Args;
public:
    PrototypeAST(const string &name, const vector<string> &args)
        : Name(name), Args(args) {}

    virtual Function *Codegen(Module *mod);
};

Function *PrototypeAST::Codegen(Module *mod) {
    // Make the function type: double(double, double) etc.
    vector<Type*> Forms(Args.size(), Type::getInt64Ty(getGlobalContext()));
    FunctionType *FT = FunctionType::get(Type::getInt64Ty(getGlobalContext()), Forms, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, mod);

    // If F conflicted, there was already something named 'Name'. If it has a
    // body, dont' allow redefinition or reextern.
    // if (F->getName() != Name) {
    //     // Delete the one we just made and get the existing one
    //     F->eraseFromParent();
    //     F = mod->getFunction(Name);

    //     // If F already has a body, reject this
    //     if (!F->empty()) {
    //         ErrorF("redefinition of a function");
    //         return 0;
    //     }

    //     // If F took a different number of args, reject.
    //     if (F->arg_size() != Args.size()) {
    //         ErrorF("redifinition of a function with different # args");
    //         return 0;
    //     }
    // }

    // // Set names for all arguments
    // unsigned Idx = 0;
    // for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size(); ++AI, ++Idx) {
    //     AI->setName(Args[Idx]);

    //     // Add arguments to variable symbol table
    //     NamedValues[Args[Idx]] = AI;
    // }
    return F;
}

// This class represents a function definition itself
class FunctionAST : public ExprCompiler {
    PrototypeAST *Proto;
    Form *Body;
public:
    FunctionAST(PrototypeAST *proto, Form *body)
        : Proto(proto), Body(body) {}

    virtual Function *Codegen(Module *mod);
};

Function *FunctionAST::Codegen(Module *mod) {
    // NamedValues.clear();

    // Function *TheFunction = Proto->Codegen();
    // if (!TheFunction) return 0;

    // BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
    // Builder.SetInsertPoint(BB);

    // if (Value *RetVal = Body->Codegen()) {
    //     // finish off the function
    //     Builder.CreateRet(RetVal);

    //     // validate the fenerate code, checking for consistency
    //     verifyFunction(*TheFunction);

    //     // optimize the function
    //     TheFPM->run(*TheFunction);
        
    //     return TheFunction;
    // }

    // TheFunction->eraseFromParent();
    return 0;
}


Compiler::Compiler() : _builder(getGlobalContext()) {
    _mod = new Module("wombat", getGlobalContext());

    string errs;
    _exec_eng = EngineBuilder(_mod).setErrorStr(&errs).create();
    if (!_exec_eng) {
        cerr << "Could not create ExecutionEngine: " << errs << endl;
        exit(1);
    }
}

Value *Compiler::form_ptr_val(Form *f) {
    return ConstantInt::get(getGlobalContext(), APInt(64, (intptr_t)f));
}

Function *Compiler::compile_top_level(Form *f) {
    FunctionType *FT = FunctionType::get(IntegerType::get(getGlobalContext(), 64), vector<Type*>(), false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, "", _mod);

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", F);
    _builder.SetInsertPoint(bb);

    Value *rval = compile(f);
    _builder.CreateRet(rval);

    verifyFunction(*F);

    return F;
}

Value *Compiler::compile(Form *f) {
    if (isa<Pair>(f))
        return compile_list(cast<Pair>(f));
    
    return form_ptr_val(f);
}

Value *Compiler::compile_list(Pair *lis) {
    Form *car = lis->car();p
    Function *fn;
    if (isa<Pair>(car))
        fn = compile_list(car);
    else if (isa<Symbol>(car)) {
        if (car == Symbol::intern("def"))
            return compile_def(lis);
        else if (car == Symbol::intern("quote"))
            return compile_quote(lis);
        fn = _mod.getNamedValue(car.name());
    }

    if (! fn)
        throw CompileError("Undefined symbol: ", car.name());
    if (! isa<Function>(fn))
        throw CompileError("Invalid function invocation.");

    return form_ptr_val(lis);
}

Value *Compiler::compile_quote(Pair *lis) {
    if (!lis->cdr())
        throw CompileError("Quote takes exactly one argument");
    if (! isa<Pair>(lis->cdr()))
        throw CompileError("Quote must be a proper list");
    Pair *quoted_form = cast<Pair>(lis->cdr());
    if (quoted_form->cdr())
        throw CompileError("Quote takes exactly one argument");

    return form_ptr_val(quoted_form->car());
}

Value *Compiler::compile_def(Pair *lis) {
    if (! lis->cdr())
        throw CompileError("def requires an argument");
    if (! isa<Pair>(lis->cdr()))
        throw CompileError("def must be a proper list");
    Pair *bind_pair = cast<Pair>(lis->cdr());
    if (! isa<Symbol>(bind_pair->car()))
        throw CompileError("def must bind to a symbol");
    if (bind_pair->cdr() && ! isa<Pair>(bind_pair->cdr()))
        throw CompileError("def must be a proper list");

    Symbol *bind_name = cast<Symbol>(bind_pair->car());
    if (_mod->getNamedValue(bind_name->name()))
        throw CompileError("symbol is already bound: ", bind_name->name());

    Value *bind_value = compile(bind_pair->cdr());
}

void *Compiler::get_fn_ptr(Function *f) {
    return _exec_eng->getPointerToFunction(f);
}

Compiler::~Compiler() {
    delete _mod;
    delete _exec_eng;
}
