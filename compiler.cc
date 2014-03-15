#include "compiler.h"

#include <memory>

Compiler::Compiler() : _builder(getGlobalContext()) {
    _mod = new Module("wombat", getGlobalContext());

    string errs;
    _exec_eng = EngineBuilder(_mod).setErrorStr(&errs).create();
    if (!_exec_eng) {
        cerr << "Could not create ExecutionEngine: " << errs << endl;
        exit(1);
    }
}

void Compiler::push_cursor(BasicBlock *bb) {
    if (BasicBlock *cur_bb = _builder.GetInsertBlock())
        _insert_pts.push_back(pair<BasicBlock*, BasicBlock::iterator>(cur_bb, _builder.GetInsertPoint()));

    _builder.SetInsertPoint(bb);
}

void Compiler::pop_cursor() {
    if (! _insert_pts.empty()) {
        auto bb_pair = _insert_pts.back();
        _builder.SetInsertPoint(bb_pair.first, bb_pair.second);
        _insert_pts.pop_back();
    }
}

Value *Compiler::form_ptr_val(Form *f) {
    return ConstantInt::get(getGlobalContext(), APInt(64, (intptr_t)f));
}

Function *Compiler::compile_top_level(Form *f) {
    return cast<Function>(compile(list3(Symbol::FN, NIL, f)));
}

Value *Compiler::resolve_local(Symbol *sym) {
    for (auto rit = _env.rbegin(); rit != _env.rend(); ++rit) {
        auto map_iter = rit->find(sym);
        if (map_iter != rit->end())
            return map_iter->second;
    }
    return nullptr;
}

void Compiler::push_local_env(map<Symbol*,Value*> env) {
    _env.push_back(env);
}
void Compiler::pop_local_env() {
    if (! _env.empty()) _env.pop_back();
}

Value *Compiler::compile(Form *f) {
    if (Pair *p = dyn_cast_or_null<Pair>(f))
        return compile_list(p);
    if (Symbol *s = dyn_cast_or_null<Symbol>(f))
        return compile_symbol(s);
    
    return form_ptr_val(f);
}

Value *Compiler::compile_symbol(Symbol *sym) {
    Value *binding = resolve_local(sym) || _mod->getNamedValue(sym->name());

    if (! binding)
        throw CompileError("Undefined symbol: ", sym->name());

    return _builder.CreateLoad(binding);
}

Value *Compiler::compile_fn(Pair *lis) {
    cerr << "In compile_fn()" << endl;
    
    Pair *body = dyn_cast_or_null<Pair>(lis->cdr());
    if (! body)
        throw CompileError("Invalid fn definition");

    Symbol *name = dyn_cast_or_null<Symbol>(body->car());
    if (name)
        body = dyn_cast_or_null<Pair>(body->cdr());
    else
        name = Symbol::intern("anon");
        
    if (! body)
        throw CompileError("Invalid fn definition");
    if (! listp(body->car()))
        throw CompileError("Function arguments must be a list");
    if (! listp(body->cdr()))
        throw CompileError("Function definition must be a proper list");

    Pair *arglist = cast_or_null<Pair>(body->car());
    vector<Symbol*> argvec;
    Symbol *a;

    while(arglist) {
        a = dyn_cast_or_null<Symbol>(arglist->car());
        if (!a) throw CompileError("Function args must be symbols");
        argvec.push_back(a);
        arglist = dyn_cast_or_null<Pair>(arglist->cdr());
    }

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry");
    push_cursor(bb);

    Pair * body_forms = cast_or_null<Pair>(body->cdr());
    Value *ret = compile_do(cons(Symbol::DO, body_forms));

    FunctionType *ft = FunctionType::get(
        ret->getType(),
        vector<Type*>(argvec.size(), Type::getInt64Ty(getGlobalContext())),
        false);
    
    Function *f = Function::Create(ft, Function::ExternalLinkage, name->name(), _mod);
    f->getBasicBlockList().insert(f->begin(), bb);

    // TODO: Create arg allocas

    _builder.CreateRet(ret);

    verifyFunction(*f);
    // TODO: Optimization passes

    pop_cursor();

    return f;
}

Value *Compiler::compile_do(Pair *lis) {
    if (! listp(lis))
        throw CompileError("'do' form must be a proper list");

    Pair *forms = cast_or_null<Pair>(lis->cdr());
    Value *rval = form_ptr_val(NIL);
    while (forms) {
        rval = compile(forms->car());
        forms = cast_or_null<Pair>(forms->cdr());
    }
    return rval;
}

Value *Compiler::compile_list(Pair *lis) {
    Form *car = lis->car();
    Value *fn = nullptr;
    if (isa<Pair>(car))
        fn = compile_list(cast<Pair>(car));
    else if (Symbol *sym = dyn_cast<Symbol>(car)) {
        if (sym == Symbol::DEF)
            return compile_def(lis);
        if (sym == Symbol::QUOTE)
            return compile_quote(lis);
        if (sym == Symbol::FN)
            return compile_fn(lis);
        if (sym == Symbol::DO)
            return compile_do(lis);
        fn = compile_symbol(sym);
    }

    // cerr << "COMPILE DEBUG: " << print_form(lis) << endl;

    PointerType *fn_type = dyn_cast<PointerType>(fn->getType());
    if (! (fn_type && fn_type->getElementType()->isFunctionTy()))
        throw CompileError("Invalid function invocation");

    if (! listp(lis))
        throw CompileError("Function call args must be a proper list.");

    Pair *args = cast_or_null<Pair>(lis->cdr());
    std::vector<Value*> argvec;
    while (args) {
        argvec.push_back(compile(args->car()));
        args = cast_or_null<Pair>(args->cdr());
    }
    
    return _builder.CreateCall(fn, argvec, fn->getName());
}

Value *Compiler::compile_quote(Pair *lis) {
    if (! lis->cdr())
        throw CompileError("Quote takes exactly one argument");
    if (! listp(lis))
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
    if (! listp(bind_pair->cdr()))
        throw CompileError("def must be a proper list");

    Symbol *bind_name = cast<Symbol>(bind_pair->car());

    Value *bind_value;
    Pair *val_form = cast_or_null<Pair>(bind_pair->cdr());
    if (val_form) {
        if (val_form->cdr())
            throw CompileError("def takes at most one value");
        bind_value = compile(val_form->car());
    } else
        bind_value = form_ptr_val(NIL);
        
    if (cast<Pair>(bind_pair->cdr())->cdr())
        throw CompileError("def requires exactly one binding value");

    Constant *gv = _mod->getNamedValue(bind_name->name());

    if (!gv)
        gv = new GlobalVariable(*_mod,
                                bind_value->getType(),
                                false,
                                GlobalValue::ExternalLinkage,
                                // value is purely external without init
                                Constant::getNullValue(bind_value->getType()),
                                bind_name->name());


    _builder.CreateStore(bind_value, gv);
    return bind_value;
}

Form *Compiler::eval(Form *input) {
    Function *f = compile_top_level(input);
    return ((Form *(*)())get_fn_ptr(f))();
}

void *Compiler::get_fn_ptr(Function *f) {
    return _exec_eng->getPointerToFunction(f);
}

