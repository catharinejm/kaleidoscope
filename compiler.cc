#include "compiler.h"
#include <sstream>

NilExpr *const NIL_EXPR = new NilExpr();

EnvList GLOBAL_DEFS;

Expr *Expr::parse(EnvList env, Form *f) {
    if (! f) return NIL_EXPR;
    if (Pair *p = dyn_cast<Pair>(f))
        if (Symbol *s = dyn_cast<Symbol>(p->car())) {
            if (s == Symbol::DEF) return DefExpr::parse(env, p);
            if (s == Symbol::FN) return FnExpr::parse(env, p);
            if (s == Symbol::QUOTE) return QuoteExpr::parse(env, p);
            if (s == Symbol::DO) return DoExpr::parse(env, p);

            return InvokeExpr::parse(env, p);
        }
    if (Number *n = dyn_cast<Number>(f))
        return NumberExpr::parse(env, n);
    if (Symbol *s = dyn_cast<Symbol>(f))
        return SymbolExpr::parse(env, s);

    throw CompileError("Unparsable form");
}

DefExpr *DefExpr::parse(EnvList env, Pair *lis) {
    if (! lis->cdr())
        throw CompileError("def requires an argument");
    if (! listp(lis->cdr()))
        throw CompileError("def must be a proper list");
    Pair *bind_pair = cast<Pair>(lis->cdr());
    if (! isa<Symbol>(bind_pair->car()))
        throw CompileError("def must bind to a symbol");
    if (count(lis) > 3)
        throw CompileError("def takes at most one binding value");
    
    DefExpr *de = new DefExpr(env, lis);
    de->_name = cast<Symbol>(bind_pair->car());
    if (Pair *valp = dyn_cast_or_null<Pair>(bind_pair->cdr()))
        de->_value = Expr::parse(env, valp->car());

    GLOBAL_DEFS.insert(EnvElem(de->_name, nullptr));
    
    return de;
}

Value *DefExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    Constant *gv = mod->getNamedValue(_name->name());

    Value *bind_value = _value->emit(C_EXPRESSION, mod, builder);
    if (!gv)
        gv = new GlobalVariable(*mod,
                                bind_value->getType(),
                                false,
                                GlobalValue::ExternalLinkage,
                                // value is purely external without init
                                Constant::getNullValue(bind_value->getType()),
                                _name->name());

    GLOBAL_DEFS[_name] = bind_value;
    return builder.CreateStore(bind_value, gv);
}

FnExpr *FnExpr::parse(EnvList env, Pair *lis) {
    Pair *body = dyn_cast_or_null<Pair>(lis->cdr());
    if (! body)
        throw CompileError("Invalid fn definition");

    FnExpr *fe = new FnExpr(env, lis);
        
    if (Symbol *name_sym = dyn_cast_or_null<Symbol>(body->car())) {
        body = dyn_cast_or_null<Pair>(body->cdr());
        fe->_name = name_sym;
    }
        
    if (! body)
        throw CompileError("Invalid fn definition");
    if (! listp(body->car()))
        throw CompileError("Function arguments must be a list");
    if (! listp(body->cdr()))
        throw CompileError("Function definition must be a proper list");

    Pair *loa = cast_or_null<Pair>(body->car());
    Symbol *a;

    while(loa) {
        a = dyn_cast_or_null<Symbol>(loa->car());
        if (!a) throw CompileError("Function args must be symbols");
        fe->_arglist.push_back(a);
        loa = dyn_cast_or_null<Pair>(loa->cdr());
    }
    Pair *body_forms = cast_or_null<Pair>(body->cdr());
    fe->_body = Expr::parse(env, cons(Symbol::DO, body_forms));

    return fe;
}

Value *FnExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    FunctionType *ft = FunctionType::get(
        TypeBuilder<void*,false>::get(getGlobalContext()),
        vector<Type*>(_arglist.size(), TypeBuilder<void*,false>::get(getGlobalContext())),
        false);
    
    Function *f = Function::Create(ft, Function::ExternalLinkage, "", mod);
    if (_name)
        _env.insert(EnvElem(_name, f));

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", f);

    auto ai = _arglist.begin();
    for (auto func_ai = f->arg_begin();
         func_ai != f->arg_end() && ai != _arglist.end();
         ++func_ai, ++ai)
    {
        func_ai->setName((*ai)->name());
        _env.insert(EnvElem(*ai, func_ai));
    }

    try {
        Value *ret = _body->emit(C_EXPRESSION, mod, builder);

        builder.CreateRet(ret);

        verifyFunction(*f);
        // TODO: Optimization passes
        return f;

    } catch (CompileError ce) {
        f->eraseFromParent();
        throw ce;
    }
}

QuoteExpr *QuoteExpr::parse(EnvList env, Pair *lis) {
    if (! listp(lis))
        throw CompileError("quote must be a proper list");
    if (count(lis) != 2)
        throw CompileError("quote takes exactly 1 argument");

    QuoteExpr *qe = new QuoteExpr(env, lis);
    qe->_quoted = dyn_cast<Pair>(lis->cdr())->car();
    return qe;
};

Value *QuoteExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    Constant *form_addr = ConstantInt::get(getGlobalContext(), APInt(64, (intptr_t) _quoted));
    return ConstantExpr::getIntToPtr(form_addr, TypeBuilder<void*,false>::get(getGlobalContext()));
}

DoExpr *DoExpr::parse(EnvList env, Pair *lis) {
    DoExpr *de = new DoExpr(env, lis);

    if (! listp(lis))
        throw CompileError("do must be a proper list");
    if (! lis->cdr()) {
        de->_ret_expr = NIL_EXPR;
        return de;
    }

    Pair *rest = dyn_cast<Pair>(lis->cdr());
    while (rest) {
        de->_statements.push_back(Expr::parse(env, rest->car()));
        rest = dyn_cast_or_null<Pair>(rest->cdr());
    }
    de->_ret_expr = de->_statements.back();
    de->_statements.pop_back();
    return de;
}

Value *DoExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    for (Expr *e : _statements)
        e->emit(C_STATEMENT, mod, builder);
    return _ret_expr->emit(ctx, mod, builder);
}

Value *NilExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    return ConstantPointerNull::get(TypeBuilder<void*,false>::get(getGlobalContext()));
}

NumberExpr *NumberExpr::parse(EnvList env, Number *n) {
    return new NumberExpr(env, n);
}

Value *NumberExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    Constant *form_addr = ConstantInt::get(getGlobalContext(), APInt(64, (intptr_t) _form));
    return ConstantExpr::getIntToPtr(form_addr, TypeBuilder<void*,false>::get(getGlobalContext()));
}

SymbolExpr *SymbolExpr::parse(EnvList env, Symbol *s) {
    if (env.find(s) == env.end())
        if (GLOBAL_DEFS.find(s) == GLOBAL_DEFS.end())
            throw CompileError("Undefined symbol: ", s->name());

    return new SymbolExpr(env, s);
}

Value *SymbolExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    if (_env.find(_sym) == _env.end() && GLOBAL_DEFS.find(_sym) == GLOBAL_DEFS.end())
        throw CompileError("CRITICAL ERROR: Undefined symbol in emit! ", _sym->name());
    
    auto v_iter = _env.find(_sym);
    if (v_iter == _env.end())
        v_iter = GLOBAL_DEFS.find(_sym);
    
    if (! v_iter->second)
        throw CompileError("Unbound symbol: ", _sym->name());
    return v_iter->second;
}

InvokeExpr *InvokeExpr::parse(EnvList env, Pair *lis) {
    if (! listp(lis))
        throw CompileError("function invokation must be a proper list");

    InvokeExpr *ie = new InvokeExpr(env, lis);
    ie->_func = Expr::parse(env, lis->car());

    Pair *rest = dyn_cast_or_null<Pair>(lis->cdr());
    while (rest) {
        ie->_params.push_back(Expr::parse(env, rest->car()));
        rest = dyn_cast_or_null<Pair>(rest->cdr());
    }
    return ie;
}

Value *InvokeExpr::emit(Expr::Context ctx, Module *mod, IRBuilder<> &builder) {
    Function *f = dyn_cast_or_null<Function>(_func->emit(C_EXPRESSION, mod, builder));
    if (! f)
        throw CompileError("Invalid function: ", print_form(_func->form()));
    if (f->arg_size() != _params.size()) {
        stringstream ss;
        ss << "Wrong number of params: " << _params.size() << " for " << f->arg_size();
        throw CompileError(ss.str());
    }

    vector<Value*> args;
    for (Expr *e : _params)
        args.push_back(e->emit(C_EXPRESSION, mod, builder));

    return builder.CreateCall(f, args);
}
