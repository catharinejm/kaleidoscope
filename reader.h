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
    virtual ~LispException() throw();
};

class ReaderError : public LispException {
public:
    ReaderError(string m) : LispException(m) {}
};

class Form : public gc {
public:
    virtual ~Form();
};

class Pair : public Form {
    Form *a, *d;
public:
    Pair(Form *_a, Form *_d) : a(_a), d(_d) {}

    Form *car() { return a; }
    Form *cdr() { return d; }    
};

class Number : public Form {
public:
    Number() {
        throw LispException("Cannot instantiate Number directly.");
    }
    virtual ~Number();
};

class Float : public Number {
    double v;
public:
    Float(double _v) : v(_v) {}
    double val() { return v; }
};

class Int : public Number {
    long v;
public:
    Int(long _v) : v(_v) {}
    long val() { return v; }
};

class Symbol : public Form {
    string n;
public:
    Symbol(string &_n) : n(_n) {}
    Symbol(const char *_n) : n(_n) {}
    const string &name() { return n; }
};

#define NIL (Pair*)0

Form *read_form(istream &input);
Pair *read_list(istream &input);
Number *read_number(istream &input);
Symbol *read_symbol(istream &input);

#endif
