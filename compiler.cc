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

Value *Compiler::compile(Form *f) {
    if (f == NIL) return 0;
    if (isa<Number>(f)) return compile_number(cast<Number>(f));

    throw CompileError("Wut?");
}

Value *Compiler::compile_number(Number *n) {
    if (isa<Int>(n))
        return ConstantInt::get(getGlobalContext(), APInt(64, n->long_val(), true));
    else if (isa<Float>(n))
        return ConstantFP::get(getGlobalContext(), APFloat(n->double_val()));

    throw CompileError("Invalid number");
}

Compiler::~Compiler() {
    delete _mod;
    delete _exec_eng;
}
