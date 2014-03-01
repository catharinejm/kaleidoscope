#include "lisp.h"
#include <stack>

Symbol *const Symbol::DEF   = Symbol::intern("def");
Symbol *const Symbol::QUOTE = Symbol::intern("quote");
Symbol *const Symbol::FN    = Symbol::intern("fn");
Symbol *const Symbol::DO    = Symbol::intern("do");

bool listp(Form *f) {
    for(;;) {
        if (!f) return true;
        Pair *p = dyn_cast<Pair>(f);
        if (!p) return false;
        f = p->cdr();
    }
}

Form *listn(Form *e1, Form *e2, Form *e3, Form *e4, Form *e5, vector<Form*> &rest) {

    Pair *tail = NIL;
    for (int i = rest.size() - 1; i >= 0; --i)
        tail = cons(rest.at(i), tail);

    return cons(e1, cons(e2, cons(e3, cons(e4, cons(e5, tail)))));
}
