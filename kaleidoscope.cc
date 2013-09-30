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
#include <map>
#include <string>
#include <vector>

using namespace llvm;

static int gettok();

// Lexer returns tokens [0-255] for unknown chars (i.e. its ASCII val), otherwise
// one of these for known things
enum Token {
    tok_eof = -1,
    // commands
    tok_def = -2, tok_extern = -3,
    // primary
    tok_identifier = -4, tok_number = -5,
    // control
    tok_if = -6, tok_then = -7, tok_else = -8,
    tok_for = -9, tok_in = -10,
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal; // Filled in if tok_number

static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}

class ExprAST;
class PrototypeAST;
class FunctionAST;

ExprAST *Error(const char *Str) { fprintf(stderr, "Error: %s\n", Str); return 0; }
PrototypeAST *ErrorP(const char *Str) { Error(Str); return 0; }
FunctionAST *ErrorF(const char *Str) { Error(Str); return 0; }
Value *ErrorV(const char *Str) { Error(Str); return 0; }

static Module *TheModule;
static IRBuilder<> Builder(getGlobalContext());
static ExecutionEngine *TheExecutionEngine;
static FunctionPassManager *TheFPM;
static std::map<std::string, Value*> NamedValues;
static std::map<char, int> BinopPrecedence;

static int gettok() {
    static int LastChar = ' ';

    // Skip whitespace
    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "def") return tok_def;
        if (IdentifierStr == "extern") return tok_extern;
        if (IdentifierStr == "if") return tok_if;
        if (IdentifierStr == "then") return tok_then;
        if (IdentifierStr == "else") return tok_else;
        if (IdentifierStr == "for") return tok_for;
        if (IdentifierStr == "in") return tok_in;
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if (LastChar == '#') {
        // Comment until EOL
        do LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
     
        if (LastChar != EOF)
            return gettok();
    }

    // Check for end of file. Don't eat the EOF
    if (LastChar == EOF)
        return tok_eof;

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

// Base class for all expression nodes
class ExprAST {
public:
    virtual ~ExprAST() {}
    virtual Value *Codegen() = 0;
};

// Expression class for numeric literals like "1.0"
class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double val) : Val(val) {}
    virtual Value *Codegen();
};

Value *NumberExprAST::Codegen() {
    return ConstantFP::get(getGlobalContext(), APFloat(Val));
}

// Expression class for referencing a variable, like "a"
class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &name) : Name(name) {}
    virtual Value *Codegen();
};

Value *VariableExprAST::Codegen() {
    Value *V = NamedValues[Name];
    return V ? V : ErrorV("Unknown variable name");
}

// Expression class for a binary operator
class BinaryExprAST : public ExprAST {
    char Op;
    ExprAST *LHS, *RHS;
public:
    BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
        : Op(op), LHS(lhs), RHS(rhs) {}

    virtual Value *Codegen();
};

Value *BinaryExprAST::Codegen() {
    Value *L = LHS->Codegen();
    Value *R = RHS->Codegen();
    if (!L || !R) return 0;

    switch (Op) {
    case '+': return Builder.CreateFAdd(L, R, "addtmp");
    case '-': return Builder.CreateFSub(L, R, "subtmp");
    case '*': return Builder.CreateFMul(L, R, "multmp");
    case '<':
        L = Builder.CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
    default: return ErrorV("invalid binary operator");
    }
}

// Expression class for function calls
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<ExprAST*> Args;
public:
    CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
        : Callee(callee), Args(args) {}

    virtual Value *Codegen();
};

Value * CallExprAST::Codegen() {
    // Look up the name in the global module table
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return ErrorV("Unknown function referenced");

    // If an argument mismatch error
    if (CalleeF->arg_size() != Args.size())
        return ErrorV("Incorrect # arguments passed");

    std::vector<Value*> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->Codegen());
        if (ArgsV.back() == 0) return 0;
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}


// Expression class for if/then/else
class IfExprAST : public ExprAST {
    ExprAST *Cond, *Then, *Else;
public:
    IfExprAST(ExprAST *cond, ExprAST *then, ExprAST *_else)
        : Cond(cond), Then(then), Else(_else) {}

    virtual Value *Codegen();
};

Value *IfExprAST::Codegen() {
    Value *CondV = Cond->Codegen();
    if (!CondV) return 0;

    // Convert condition to a bool bu comparing equal to 0.0.
    CondV = Builder.CreateFCmpONE(CondV, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "ifcond");

    Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Create blocks for the 'then' and 'else' cases. Insert the 'then' block at
    // the end of the function
    BasicBlock *ThenBB = BasicBlock::Create(getGlobalContext(), "then", TheFunction);
    BasicBlock *ElseBB = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock *MergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit the value
    Builder.SetInsertPoint(ThenBB);

    Value *ThenV = Then->Codegen();
    if (!ThenV) return 0;

    Builder.CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB for the PHI
    ThenBB = Builder.GetInsertBlock();

    // Emit else block
    TheFunction->getBasicBlockList().push_back(ElseBB);
    Builder.SetInsertPoint(ElseBB);

    Value *ElseV = Else->Codegen();
    if (!ElseV) return 0;

    Builder.CreateBr(MergeBB);
    // Codegen of Else can change the current block, update ElseBB for the PHI
    ElseBB = Builder.GetInsertBlock();

    // Emit merge block
    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    PHINode *PN = Builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 0, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

// Expression class for for/in
class ForExprAST : public ExprAST {
    std::string VarName;
    ExprAST *Start, *End, *Step, *Body;
public:
    ForExprAST(const std::string &varname, ExprAST *start, ExprAST *end,
               ExprAST *step, ExprAST *body)
        : VarName(varname), Start(start), End(end), Step(step), Body(body) {}
    virtual Value *Codegen();
};

Value *ForExprAST::Codegen() {
    // Emit the start code first without 'variable' in scope
    Value *StartVal = Start->Codegen();
    if (!StartVal) return 0;

    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock *PreheaderBB = Builder.GetInsertBlock();
    BasicBlock *LoopBB = BasicBlock::Create(getGlobalContext(), "loop", TheFunction);

    // Insert an explicit fall through from the current block to LoopBB
    Builder.CreateBr(LoopBB);

    // Start insertion in LoopBB
    Builder.SetInsertPoint(LoopBB);

    // Start the PHI node with an entry for Start
    PHINode *Variable = Builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 2, VarName.c_str());
    Variable->addIncoming(StartVal, PreheaderBB);

    // Within the loop, the variable is defined equal to the PHI node. If it
    // shadoes an existing variable, we have to restore it, so save it now.
    Value *OldVal = NamedValues[VarName];
    NamedValues[VarName] = Variable;

    // Emit the body of the loop. This, like any other expr, can change the
    // current BB. Node that we ignore the value computed by the body, but don't
    // allow an error.
    if (!Body->Codegen()) return 0;

    // Emit the step value.
    Value *StepVal;
    if (Step) {
        StepVal = Step->Codegen();
        if (!StepVal) return 0;
    } else {
        // If not specified, use 1.0
        StepVal = ConstantFP::get(getGlobalContext(), APFloat(1.0));
    }

    Value *NextVar = Builder.CreateFAdd(Variable, StepVal, "nextvar");

    // Compute the end condition
    Value *EndCond = End->Codegen();
    if (!EndCond) return 0;

    // convert condition to a bool by comparing equal to 0.0
    EndCond = Builder.CreateFCmpONE(EndCond, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "loopcond");

    // Create the "after loop" block and insert it
    BasicBlock *LoopEndBB = Builder.GetInsertBlock();
    BasicBlock *AfterBB = BasicBlock::Create(getGlobalContext(), "afterloop", TheFunction);

    // insert the conditional branch into the end of LoopEndBB
    Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

    // any new code will be inserted in AfterBB
    Builder.SetInsertPoint(AfterBB);

    // Add a new entry to the PHI node for the backedge
    Variable->addIncoming(NextVar, LoopEndBB);

    // Restore the unshadowed variable.
    if (OldVal)
        NamedValues[VarName] = OldVal;
    else
        NamedValues.erase(VarName);

    // for expr always returns 0.0
    return Constant::getNullValue(Type::getDoubleTy(getGlobalContext()));
}

// This class represents the "prototype" for a function, which captures its name
// and its arguments names (thus implicitly the number of arguments the function
// takes).
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string &name, const std::vector<std::string> &args)
        : Name(name), Args(args) {}

    virtual Function *Codegen();
};

Function *PrototypeAST::Codegen() {
    // Make the function type: double(double, double) etc.
    std::vector<Type*> Doubles(Args.size(),
                               Type::getDoubleTy(getGlobalContext()));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()),
                                         Doubles, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);

    // If F conflicted, there was already something named 'Name'. If it has a
    // body, dont' allow redefinition or reextern.
    if (F->getName() != Name) {
        // Delete the one we just made and get the existing one
        F->eraseFromParent();
        F = TheModule->getFunction(Name);

        // If F already has a body, reject this
        if (!F->empty()) {
            ErrorF("redefinition of a function");
            return 0;
        }

        // If F took a different number of args, reject.
        if (F->arg_size() != Args.size()) {
            ErrorF("redifinition of a function with different # args");
            return 0;
        }
    }

    // Set names for all arguments
    unsigned Idx = 0;
    for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size(); ++AI, ++Idx) {
        AI->setName(Args[Idx]);

        // Add arguments to variable symbol table
        NamedValues[Args[Idx]] = AI;
    }
    return F;
}

// This class represents a function definition itself
class FunctionAST {
    PrototypeAST *Proto;
    ExprAST *Body;
public:
    FunctionAST(PrototypeAST *proto, ExprAST *body)
        : Proto(proto), Body(body) {}

    virtual Function *Codegen();
};

Function *FunctionAST::Codegen() {
    NamedValues.clear();

    Function *TheFunction = Proto->Codegen();
    if (!TheFunction) return 0;

    BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
    Builder.SetInsertPoint(BB);

    if (Value *RetVal = Body->Codegen()) {
        // finish off the function
        Builder.CreateRet(RetVal);

        // validate the fenerate code, checking for consistency
        verifyFunction(*TheFunction);

        // optimize the function
        TheFPM->run(*TheFunction);
        
        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return 0;
}

static ExprAST *ParsePrimary();

static int GetTokPrecedence() {
    if (!isascii(CurTok))
        return -1;

    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) return -1;
    return TokPrec;
}

static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
    // if this is binop, find its precedence
    for (;;) {
        int TokPrec = GetTokPrecedence();

        // if this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done
        if (TokPrec < ExprPrec)
            return LHS;

        // Okay we now this is a binop
        int BinOp = CurTok;
        getNextToken(); // eat binop

        ExprAST *RHS = ParsePrimary();
        if (!RHS) return 0;

        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec+1, RHS);
            if (RHS == 0) return 0;
        }
        // Merge LHS/RHS
        LHS = new BinaryExprAST(BinOp, LHS, RHS);
    }
}

// expression
//   ::= primary binoprhs
static ExprAST *ParseExpression() {
    ExprAST *LHS = ParsePrimary();
    if (!LHS) return 0;

    return ParseBinOpRHS(0, LHS);
}

// ifexpr ::= 'if' expression 'then' expression 'else' expression
static ExprAST *ParseIfExpr() {
    getNextToken(); // eat the 'if'

    // condition
    ExprAST *Cond = ParseExpression();
    if (!Cond) return 0;

    if (CurTok != tok_then)
        return Error("expected then");
    getNextToken(); // eat the 'then'

    ExprAST *Then = ParseExpression();
    if (!Then) return 0;

    if (CurTok != tok_else)
        return Error("Expected else");
    getNextToken(); // eat the 'else'

    ExprAST *Else = ParseExpression();
    if (!Else) return 0;

    return new IfExprAST(Cond, Then, Else);
}

// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
static ExprAST *ParseForExpr() {
    getNextToken(); // eat the 'for'

    if (CurTok != tok_identifier)
        return Error("expected identifier after 'for'");

    std::string IdName = IdentifierStr;
    getNextToken(); // eat identifier

    if (CurTok != '=')
        return Error("expected '=' after 'for'");
    getNextToken(); // eat '='

    ExprAST *Start = ParseExpression();
    if (!Start) return 0;
    if (CurTok != ',')
        return Error("expected ',' after for start value");
    getNextToken(); // eat '='
    
    ExprAST *End = ParseExpression();
    if (!End) return 0;

    // The step value is optional
    ExprAST *Step = 0;
    if (CurTok == ',') {
        getNextToken();
        Step = ParseExpression();
        if (!Step) return 0;
    }

    if (CurTok != tok_in)
        return Error("expected 'in' after 'for'");
    getNextToken(); // eat 'in'

    ExprAST *Body = ParseExpression();
    if (!Body) return 0;

    return new ForExprAST(IdName, Start, End, Step, Body);
}

// numberexpr ::= number
static ExprAST *ParseNumberExpr() {
    ExprAST *Result = new NumberExprAST(NumVal);
    getNextToken();
    return Result;
}

// prenexpr ::= '(' expression ')'
static ExprAST *ParseParenExpr() {
    getNextToken(); // eat '('
    ExprAST *V = ParseExpression();
    if (!V) return 0;

    if (CurTok != ')')
        return Error("expected ')'");
    getNextToken(); // eat ')'
    return V;
}

// identifierexpr
//   ::= identifier
//   ::= identifier '(' expression* ')'
static ExprAST *ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken(); // eat identifier

    if (CurTok != '(') // Simple variable ref.
        return new VariableExprAST(IdName);

    // Call
    getNextToken(); // eat '('
    std::vector<ExprAST*> Args;
    if (CurTok != ')') {
        for(;;) {
            ExprAST *Arg = ParseExpression();
            if (!Arg) return 0;
            Args.push_back(Arg);

            if (CurTok == ')') break;

            if (CurTok != ',')
                return Error("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    // Eat the ')'
    getNextToken();

    return new CallExprAST(IdName, Args);
}

// primary
//   ::= identifierexpr
//   ::= numberexpr
//   ::= parenexpr
static ExprAST *ParsePrimary() {
    switch (CurTok) {
    case tok_identifier: return ParseIdentifierExpr();
    case tok_number: return ParseNumberExpr();
    case '(': return ParseParenExpr();
    case tok_if: return ParseIfExpr();
    case tok_for: return ParseForExpr();
    default: return Error("unknown token when expecting an expression");
    }
}

// prototype
//   ::= id '(' id* ')'
static PrototypeAST *ParsePrototype() {
    if (CurTok != tok_identifier)
        return ErrorP("Expected function name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return ErrorP("Expected '(' in prototype");

    // Read the list of argument names
    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);
    if (CurTok != ')')
        return ErrorP("Expected ')' in prototype");

    // success!
    getNextToken(); // eat ')'

    return new PrototypeAST(FnName, ArgNames);
}

static FunctionAST *ParseDefinition() {
    getNextToken(); // eat def.
    PrototypeAST *Proto = ParsePrototype();
    if (Proto == 0) return 0;

    if (ExprAST *E = ParseExpression())
        return new FunctionAST(Proto, E);
    return 0;
}

// external ::= 'extern' prototype
static PrototypeAST *ParseExtern() {
    getNextToken(); // eat extern.
    return ParsePrototype();
}

// toplevelexpr ::= expression
static FunctionAST *ParseTopLevelExpr() {
    if (ExprAST *E = ParseExpression()) {
        // Make an anonymous proto
        PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
        return new FunctionAST(Proto, E);
    }
    return 0;
}

static void HandleDefinition() {
    if (FunctionAST * F = ParseDefinition()) {
        if (Function *LF = F->Codegen()) {
            fprintf(stderr, "Read function definition:");
            LF->dump();
        }
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleExtern() {
    if (PrototypeAST *P = ParseExtern()) {
        if (Function *F = P->Codegen()) {
            fprintf(stderr, "Read extern:");
            F->dump();
        }
    } else {
        // skip token for errror recovery
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate top-leve expression into an anonymous function
    if (FunctionAST *F = ParseTopLevelExpr()) {
        if (Function *LF = F->Codegen()) {
            LF->dump(); // Dump the function for exposition purposes

            // JIT the function, returning a function pointer
            void *FPtr = TheExecutionEngine->getPointerToFunction(LF);

            // Cast it to the right type (takes no arguments, returns a double)
            // so we can call it as a native function
            double (*FP)() = (double (*)())(intptr_t)FPtr;
            fprintf(stderr, "Evaluated to %f\n", FP());
        }
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

static void MainLoop() {
    for (;;) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
        case tok_eof: return;
        case ';': getNextToken(); break; // ignore top-level semicolons
        case tok_def: HandleDefinition(); break;
        case tok_extern: HandleExtern(); break;
        default: HandleTopLevelExpression(); break;
        }
    }
}

extern "C"
double putchard(double X) {
    putchar((char)X);
    return 0;
}

int main() {
    InitializeNativeTarget();
    LLVMContext &Context = getGlobalContext();
    
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    // Prime the first token
    fprintf(stderr, "ready> ");
    getNextToken();

    // Make the moodule, which holds all the code
    TheModule = new Module("my cool jit", Context);

    std::string ErrStr;
    TheExecutionEngine = EngineBuilder(TheModule).setErrorStr(&ErrStr).create();
    if (!TheExecutionEngine) {
        fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
        exit(1);
    }

    FunctionPassManager OurFPM(TheModule);

    // Set up the optimizer pipeline. Start with registering info about how the
    // target lays out data structures.
    OurFPM.add(new DataLayout(*TheExecutionEngine->getDataLayout()));
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

    // Run the main "interpreter loop" now.
    MainLoop();

    TheFPM = 0;

    TheModule->dump();

    return 0;
}
