#include "lisp.h"

#include <sstream>

string print_form(Form *form) {
    if (nilp(form))
        return "()";
    if (pairp(form))
        return string("(") + print_list(as_pair(form)) + ")";
    if (symbolp(form))
        return print_symbol(as_symbol(form));
    if (intp(form))
        return print_int(as_int(form));
    if (floatp(form))
        return print_float(as_float(form));

    throw TypeError("Unknown form type", form);
}

string print_list(Pair *form) {
    string listr = print_form(form->car());
    if (nilp(form->cdr())) return listr;
    if (pairp(form->cdr()))
        return listr + " " + print_list(as_pair(form->cdr()));

    return listr + " . " + print_form(form->cdr());
}

string print_symbol(Symbol *sym) {
    return sym->name();
}

string print_number(Number *n) {
    if (intp(n))
        return print_int(as_int(n));
    if (floatp(n))
        return print_float(as_float(n));

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
