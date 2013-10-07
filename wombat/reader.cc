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

#include <cstdio>
#include <cctype>
#include <string>
#include <map>
#include <set>

using namespace std;
using namespace llvm;

class WVal;
class Cons;

enum WTYPE { NIL_TYPE = 0, DOUBLE, SYMBOL, CONS };
union WVAL {
    double d;
    void *p;
};

class WVal {
public:
    WTYPE ty;
    WVAL val;

    WVal() {}
    WVal(WTYPE t) : ty(t) {}
    WVal(WTYPE t, WVAL v) : ty(t), val(v) {}
    WVal(const WVal &other) : ty(other.ty), val(other.val) {}
};

const WVal NIL(NIL_TYPE, (WVAL){0});

class Cons : public WVal {
    WVal head, tail;
public:
    Cons(WVal hd, WVal tl) : WVal(CONS), head(hd), tail(tl) {
        val.p = (void*)this;
    }
    Cons(const Cons &other) : WVal(other), head(other.head), tail(other.tail) {}
};

class Symbol : public WVal {
    Symbol(const string *np) : WVal(SYMBOL) {
        val.p = (void*)np;
    }
    Symbol(const string &n) : Symbol(&n) {}

public:
    static Symbol intern(const string &name) {
        static set<string> intern_set;
        auto rec = intern_set.insert(name);
        Symbol rval(*rec.first);
        return rval;
    }
    bool operator==(const Symbol &other) { return val.p == other.val.p; }
    const string &name() const { return *(string*)val.p; }
};

void eat_whitespace(FILE * io) {
    int cur;
    do cur = getc(io);
    while (isspace(cur));
    ungetc(cur, io);
}

WVal read_list(FILE * io) {
    int cur;
    eat_whitespace(io);

    cur = getc(io);
    if (cur == ')')
        return NIL;

    ungetc(cur, io);
    WVal head, tail;
    head = read_form(io);
    eat_whitespace(io);

    cur = getc(io);
    if (cur == '.')
        tail = read_form(io);
    else
        tail = read_list(io);

    return Cons(head, tail);
}

WVal read_form(FILE * io) {
    int cur;
    eat_whitespace(io);
    cur = getc(io);

    if (cur == EOF) {
        fprintf(stderr, "^D");
        exit(0);
    }

    if (cur == '(')
        return read_list(io);
    if (isdigit(cur))
        return read_number(io);
    return read_symbol(io);
}

int main() {
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    
    // Make the moodule, which holds all the code
    TheModule = new Module("wombat", Context);

    string ErrStr;
    TheExecutionEngine = EngineBuilder(TheModule).setErrorStr(&ErrStr).create();
    if (!TheExecutionEngine) {
        fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
        exit(1);
    }

/*
  FunctionPassManager OurFPM(TheModule);

  // Set up the optimizer pipeline. Start with registering info about how the
  // target lays out data structures.
  OurFPM.add(new DataLayout(*TheExecutionEngine->getDataLayout()));
  // Promote allocas to registers
  OurFPM.add(createPromoteMemoryToRegisterPass());
  // Provide basic AliasAnalysis support for GVN.
  OurFPM.add(createBasicAliasAnalysisPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  OurFPM.add(createInstructionCombiningPass());
  // Reassociate expressions.
  OurFPM.add(createReassociatePass());
  // Eliminate Common Subexpressions
  OurFPM.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  OurFPM.add(createCFGSimplificationPass());

  OurFPM.doInitialization();

  // set the global so the code gen can use this
  TheFPM = &OurFPM;
*/
  
    printf("> ");

    for (;;) {
        WVal form = read_form(stdin);
        printf("%s\n", line.c_str());
    }
    
//    TheFPM = 0;
    
    TheModule->dump();

    return 0;
}
