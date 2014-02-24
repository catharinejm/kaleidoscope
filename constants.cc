#include "lisp.h"

Symbol *const Symbol::DEF   = Symbol::intern("def");
Symbol *const Symbol::QUOTE = Symbol::intern("quote");
Symbol *const Symbol::FN    = Symbol::intern("fn");

bool listp(Form *f) {
    if (!f) return true;
    if (! isa<Pair>(f)) return false;
    while (f->cdr())
        if (! (f = dyn_cast<Pair>(f->cdr()))) return false;
    return true;
}
