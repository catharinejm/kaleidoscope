#include "lisp.h"

#include <sstream>

string print_form(Form *form) {
    if (form == NIL)
        return "()";
    if (isa<Pair>(form))
        return string("(") + print_list(cast<Pair>(form)) + ")";
    if (isa<Symbol>(form))
        return print_symbol(cast<Symbol>(form));
    if (isa<Int>(form))
        return print_int(cast<Int>(form));
    if (isa<Float>(form))
        return print_float(cast<Float>(form));

    throw TypeError("Unknown form type", form);
}

string print_list(Pair *form) {
    string listr = print_form(form->car());
    if (form->cdr() == NIL) return listr;
    if (isa<Pair>(form->cdr()))
        return listr + " " + print_list(cast<Pair>(form->cdr()));

    return listr + " . " + print_form(form->cdr());
}

string print_symbol(Symbol *sym) {
    return sym->name();
}

string print_number(Number *n) {
    if (isa<Int>(n))
        return print_int(cast<Int>(n));
    if (isa<Float>(n))
        return print_float(cast<Float>(n));

    throw TypeError("Unknown number type", n);
}

string print_int(Int *i) {
    ostringstream intstr;
    intstr << dec << i->long_val();
    return intstr.str();
}

string print_float(Float *f) {
    ostringstream floatstr;
    floatstr << f->double_val();
    return floatstr.str();
}
