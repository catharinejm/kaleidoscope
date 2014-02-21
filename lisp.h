// -*- mode: c++ -*-
#ifndef _WOMBAT_READER_H
#define _WOMBAT_READER_H

#include <iostream>
#include <exception>

#include <gc/gc_cpp.h>

using namespace std;

class LispException : public exception, public gc {
    string msg;
public:
    LispException(string m) : msg(m) {}
    virtual const char *what() { return msg.c_str(); }
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
    virtual ~Form() {};
};

class Pair : public Form {
    Form *a, *d;
public:
    Pair(Form *_a, Form *_d) : a(_a), d(_d) {}

    Form *car() { return a; }
    Form *cdr() { return d; }    
};

class Number : public Form {
protected:
    Number() {}
public:
    virtual long long_val() = 0;
    virtual double double_val() = 0;
};

class Float : public Number {
    double val;
public:
    Float(double d) : val(d) {}
    virtual long long_val() { return (long)val; }
    virtual double double_val() { return val; }
};

class Int : public Number {
    long val;
public:
    Int(long l) : val(l) {}
    virtual long long_val() { return val; }
    virtual double double_val() { return (double)val; }
};

class Symbol : public Form {
    string n;
public:
    Symbol(string &_n) : n(_n) {}
    Symbol(const char *_n) : n(_n) {}
    const string &name() { return n; }
};

#define NIL (Form*)0

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

inline bool nilp(Form *f) { return f == NIL; }
inline bool pairp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Pair); }
inline bool symbolp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Symbol); }
inline bool intp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Int); }
inline bool floatp(Form *f) { return !nilp(f) && typeid(*f) == typeid(Float); }
inline bool numberp(Form *f) { return intp(f) || floatp(f); }

inline Pair *as_pair(Form *f) { return static_cast<Pair*>(f); }
inline Symbol *as_symbol(Form *f) { return static_cast<Symbol*>(f); }
inline Int *as_int(Form *f) { return static_cast<Int*>(f); }
inline Float *as_float(Form *f) { return static_cast<Float*>(f); }

inline Pair *cons(Form *a, Form *d) { return new Pair(a, d); }

static void _dbg(Form *f) { cerr << "DEBUG: " << typeid(*f).name() << endl; }

#endif
