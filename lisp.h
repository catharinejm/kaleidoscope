// -*- mode: c++ -*-xcep
#ifndef _WOMBAT_READER_H
#define _WOMBAT_READER_H

#include "llvm/Support/Casting.h"
#include "llvm/IR/Function.h"

#include <iostream>
#include <exception>
#include <unordered_map>
#include <vector>

#include <gc/gc_cpp.h>

using namespace std;
using namespace llvm;

class LispException : public exception, public gc {
    string msg;
public:
    LispException(string m) : msg(m) {}
    virtual const char *what() const noexcept { return msg.c_str(); }
    virtual ~LispException() throw() {};
};

class ReaderError : public LispException {
public:
    ReaderError(string m) : LispException(m) {}
    ReaderError(string m, string rest) : LispException(m + rest) {}
};

class Form;

class TypeError : public LispException {
    Form *obj;
public:
    TypeError(string m) : LispException(m) {}
    TypeError(string m, Form *o) : LispException(m), obj(o) {}

    Form *culprit() { return obj; }
};

class Form : public gc {
public:
    enum FormKind {
        FK_Symbol,
        FK_Pair,

        FK_Number,
        FK_Float,
        FK_Int,
        FK_NumberEnd,

        FK_Fn,
    };
    
    virtual ~Form() {};

    const FormKind getKind() const { return _kind; }

private:
    const FormKind _kind;
protected:
    Form(FormKind fk) : _kind(fk) {}
};

class Pair : public Form {
    Form *_a, *_d;
public:
    Pair(Form *a, Form *d) : Form(FK_Pair), _a(a), _d(d) {}

    static bool classof(const Form *f) { return f->getKind() == FK_Pair; }

    Form *car() { return _a; }
    Form *cdr() { return _d; }
    void setcar(Form *a) { _a = a; }
    void setcdr(Form *d) { _d = d; }
};

class Number : public Form {
protected:
    Number(FormKind _k) : Form(_k) {}
public:
    virtual long long_val() = 0;
    virtual double double_val() = 0;

    static bool classof(const Form *f) {
        return f->getKind() >= FK_Number && f->getKind() <= FK_NumberEnd;
    }
};

class Float : public Number {
    double val;
public:
    Float(double d) : Number(FK_Float), val(d) {}
    virtual long long_val() { return (long)val; }
    virtual double double_val() { return val; }

    static bool classof(const Form *f) { return f->getKind() == FK_Float; }
};

class Int : public Number {
    long val;
public:
    Int(long l) : Number(FK_Int), val(l) {}
    virtual long long_val() { return val; }
    virtual double double_val() { return (double)val; }

    static bool classof(const Form *f) { return f->getKind() == FK_Int; }
};

class Symbol : public Form {
    string n;
protected:    
    Symbol(string &_n) : Form(FK_Symbol), n(_n) {}
    Symbol(const char *_n) : Form(FK_Symbol), n(_n) {}

public:
    static Symbol *intern(string name) {
        static unordered_map<string, Symbol*> _syms;
        auto s_iter = _syms.find(name);
        if (s_iter == _syms.end()) {
            auto res_pair = _syms.insert(pair<string, Symbol*>(name, new Symbol(name)));
            if (res_pair.second)
                return res_pair.first->second;
            throw LispException("Failed to intern symbol.");
        }
        return s_iter->second;
    }
    const string &name() { return n; }

    static bool classof(const Form *f) { return f->getKind() == FK_Symbol; }

    static Symbol *const DEF;
    static Symbol *const QUOTE;
    static Symbol *const FN;
    static Symbol *const DO;
};

class Fn : public Form {
    Pair *_src;
    Function *_fn;
public:
    Fn(Pair *s, Function *f) : Form(FK_Fn), _src(s), _fn(f) {}

    static bool classof(const Form *f) { return f->getKind() == FK_Fn; }

    Pair *src() { return _src; }
    Function *fn() { return _fn; }
};

#define NIL nullptr

Form *read_form(istream &input);
Pair *read_list(istream &input);
Form *read_number(istream &input);
Symbol *read_symbol(istream &input);

string print_form(Form *form);
string print_list(Pair *pair);
string print_number(Number *n);
string print_int(Int *i);
string print_float(Float *i);
string print_symbol(Symbol *s);

extern "C" {
    bool listp(Form *p);

    Form *invoke(Fn *fn);

    inline Pair *cons(Form *a, Form *d) { return new Pair(a, d); }

    inline Form *list1(Form *elem) { return cons(elem, NIL); }
    inline Form *list2(Form *e1, Form *e2) { return cons(e1, cons(e2, NIL)); }
    inline Form *list3(Form *e1, Form *e2, Form *e3) { return cons(e1, cons(e2, cons(e3, NIL))); }
    inline Form *list4(Form *e1, Form *e2, Form *e3, Form *e4) { return cons(e1, cons(e2, cons(e3, cons(e4, NIL)))); }
    inline Form *list5(Form *e1, Form *e2, Form *e3, Form *e4, Form *e5) { return cons(e1, cons(e2, cons(e3, cons(e4, cons(e5, NIL))))); }

    Form *listn(Form *e1, Form *e2, Form *e3, Form *e4, Form *e5, vector<Form*> &rest);

    int count(Pair *p);
}

// inline bool nilp(Form *f) { return f == NIL; }
// inline bool pairp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Pair); }
// inline bool symbolp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Symbol); }
// inline bool intp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Int); }
// inline bool floatp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Float); }
// inline bool numberp(Form *f) { return intp(f) || floatp(f); }

// inline Pair *as_pair(Form *f) { return static_cast<Pair*>(f); }
// inline Symbol *as_symbol(Form *f) { return static_cast<Symbol*>(f); }
// inline Int *as_int(Form *f) { return static_cast<Int*>(f); }
// inline Float *as_float(Form *f) { return static_cast<Float*>(f); }

#endif
