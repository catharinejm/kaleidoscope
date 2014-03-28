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

typedef unordered_map<Symbol*,Value*> EnvList;
typedef pair<Symbol*,Value*> EnvElem;

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
 
    static Expr *parse(EnvList env, Form *f);

    virtual Form *form() = 0;
    virtual Pair *pair() { return dyn_cast_or_null<Pair>(form()); }
    virtual Symbol *symbol() { return dyn_cast_or_null<Symbol>(form()); }
    virtual Value *emit(Context ctx, Module *mod, IRBuilder<> &builder) = 0;
private:
    ExprKind _kind;
    EnvList _env;
protected:
    Expr(EnvList e, ExprKind ek) : _env(e), _kind(ek) {}
};

class DefExpr : public Expr {
    Pair *_form;

    Symbol *_name;
    Expr *_value;

    DefExpr(EnvList e, Pair *p) : Expr(e, EK_DefExpr), _form(p) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_DefExpr; }
    static DefExpr *parse(EnvList env, Pair *lis);

    virtual Form *form() { return _form; }
};

class FnExpr : public Expr {
    Pair *_form;

    Symbol *_name;
    vector<Symbol*> _arglist;
    Expr *_body;

    FnExpr(EnvList e, Pair *p) : Expr(e, EK_FnExpr), _form(p) {}
    
public:
    static bool classof(const Expr *e) { return e->getKind() == EK_FnExpr; }
    static FnExpr *parse(EnvList env, Pair *lis);

    virtual Form *form() { return _form; }
};

class QuoteExpr : public Expr {
    Pair *_form;

    Form *_quoted;
    QuoteExpr(EnvList e, Pair *p) : Expr(e, EK_QuoteExpr), _form(p) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_QuoteExpr; }
    static QuoteExpr *parse(EnvList env, Pair *lis);

    virtual Form *form() { return _form; }
};

class DoExpr : public Expr {
    Pair *_form;

    vector<Expr*> _statements;
    Expr *_ret_expr;

    DoExpr(EnvList e, Pair *p) : Expr(e, EK_DoExpr), _form(p) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_DoExpr; }
    static DoExpr *parse(EnvList e, Pair *lis);

    virtual Form *form() { return _form; }
};

class NilExpr : public Expr {
public:
    NilExpr() : Expr(EnvList(), EK_NilExpr) {}

    virtual Form *form() { return nullptr; }
};

class NumberExpr : public Expr {
    Number *_form;

    NumberExpr(EnvList e, Number *n) : Expr(e, EK_NumberExpr), _form(n) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_NumberExpr; }
    static NumberExpr *parse(EnvList e, Number *n);

    virtual Form *form() { return _form; }
};

class SymbolExpr : public Expr {
    Symbol *_sym;

    SymbolExpr(EnvList e, Symbol *s) : Expr(e, EK_SymbolExpr), _sym(s) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_SymbolExpr; }
    static SymbolExpr *parse(EnvList e, Symbol *s);

    virtual Form *form() { return _sym; }
};

class InvokeExpr : public Expr {
    Pair *_form;

    Expr *_func;
    vector<Expr*> _params;

    InvokeExpr(EnvList e, Pair *lis) : Expr(e, EK_InvokeExpr), _form(lis) {}

public:
    static bool classof(const Expr *e) { return e->getKind() == EK_InvokeExpr; }
    static InvokeExpr *parse(EnvList e, Symbol *s);

    virtual Form *form() { return _sym; }
};

#endif
