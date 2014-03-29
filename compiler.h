// -*- mode: c++ -*-
#ifndef WOMBAT_COMPILER_H
#define WOMBAT_COMPILER_H

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"

#include "lisp.h"

#include <vector>
#include <unordered_map>

using namespace llvm;
using namespace std;

class CompileError : public LispException {
public:
    CompileError(string m) : LispException(m) {}
    CompileError(string m, string n) : LispException(m + n) {}
};

typedef unordered_map<Symbol*,Value*> EnvMap;
typedef pair<Symbol*,Value*> EnvElem;
typedef vector<EnvMap> EnvList;

class Expr : public gc {
public:
    enum ExprKind {
        EK_DefExpr,
        EK_QuoteExpr,
        EK_FnExpr,
        EK_DoExpr,
        EK_NilExpr,
        EK_NumberExpr,
        EK_SymbolExpr,
        EK_InvokeExpr,
    };

    enum Context {
        C_STATEMENT,  //value ignored
        C_EXPRESSION, //value required
        C_RETURN,     //tail position relative to enclosing recur frame
        C_EVAL,
    };

    virtual ~Expr() {};
    const ExprKind getKind() const { return _kind; }
 
    static Expr *parse(Form *f);

    virtual Form *form() = 0;
    virtual Pair *pair() { return dyn_cast_or_null<Pair>(form()); }
    virtual Symbol *symbol() { return dyn_cast_or_null<Symbol>(form()); }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder) = 0;

protected:
    ExprKind _kind;

    Expr(ExprKind ek) : _kind(ek) {}
};

class DefExpr : public Expr {
    Pair *_form;

    Symbol *_name;
    Expr *_value;

    DefExpr(Pair *p) : Expr(EK_DefExpr), _form(p) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_DefExpr; }
    static DefExpr *parse(Pair *lis);

    virtual Form *form() { return _form; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

class FnExpr : public Expr {
    Pair *_form;

    Symbol *_name;
    vector<Symbol*> _arglist;
    Expr *_body;

    EnvList::reverse_iterator _env_i;

    FnExpr(Pair *p) : Expr(EK_FnExpr), _form(p) {}
    
public:
    static bool classof(const Expr *e) { return e->getKind() == EK_FnExpr; }
    static FnExpr *parse(Pair *lis);

    virtual Form *form() { return _form; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

class QuoteExpr : public Expr {
    Pair *_form;

    Form *_quoted;
    QuoteExpr(Pair *p) : Expr(EK_QuoteExpr), _form(p) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_QuoteExpr; }
    static QuoteExpr *parse(Pair *lis);

    virtual Form *form() { return _form; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

class DoExpr : public Expr {
    Pair *_form;

    vector<Expr*> _statements;
    Expr *_ret_expr;

    DoExpr(Pair *p) : Expr(EK_DoExpr), _form(p) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_DoExpr; }
    static DoExpr *parse(Pair *lis);

    virtual Form *form() { return _form; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

class NilExpr : public Expr {
public:
    NilExpr() : Expr(EK_NilExpr) {}

    virtual Form *form() { return nullptr; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

class NumberExpr : public Expr {
    Number *_form;

    NumberExpr(Number *n) : Expr(EK_NumberExpr), _form(n) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_NumberExpr; }
    static NumberExpr *parse(Number *n);

    virtual Form *form() { return _form; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

class SymbolExpr : public Expr {
    Symbol *_sym;

    SymbolExpr(Symbol *s) : Expr(EK_SymbolExpr), _sym(s) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_SymbolExpr; }
    static SymbolExpr *parse(Symbol *s);

    virtual Form *form() { return _sym; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

class InvokeExpr : public Expr {
    Pair *_form;

    Expr *_func;
    vector<Expr*> _params;

    InvokeExpr(Pair *lis) : Expr(EK_InvokeExpr), _form(lis) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_InvokeExpr; }
    static InvokeExpr *parse(Pair *s);

    virtual Form *form() { return _form; }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder);
};

#endif
