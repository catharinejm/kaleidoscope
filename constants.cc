#include "lisp.h"

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
