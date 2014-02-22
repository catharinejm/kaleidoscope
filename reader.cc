#include "lisp.h"

#include <cctype>
#include <sstream>

inline bool is_whitespace(char c) {
    return isspace(c) || c == ',';
}

inline char killws(istream &input) {
    char c;
    while (is_whitespace(c = input.get()));
    return c;
}

inline bool is_sym_char(char c) {
    return !is_whitespace(c) && c != '(' && c != ')';
}

Form *read_number(istream &input) {
    stringstream num_stream;
    char cur = input.get(), sign = '\0';
    num_stream << cur;
    
    if (cur == '-' || cur == '+') {
        sign = cur;
        cur = input.get();
        num_stream << cur;
    }
        
    if (cur == '0') {
        if (!is_sym_char(input.peek()))
            return new /*(NoGC)*/ Int(0);
        
        cur = input.get();
        num_stream << cur;
        
        char dispatch = cur;
                
        while (is_sym_char(cur = input.get()))
            num_stream << cur;
        input.putback(cur);

        Number *rval;
        if (dispatch == '.') {
            double d;
            num_stream >> d;
            rval = new /*(NoGC)*/ Float(d);
        } else {
            long l;

            if (dispatch == 'x' || dispatch == 'X')
                num_stream >> hex >> l;
            else if (isdigit(dispatch))
                num_stream >> oct >> l;

            rval = new /*(NoGC)*/ Int(l);
        }

        if (!num_stream.eof())
            throw ReaderError("Invalid number format: ", num_stream.str());

        return rval;
    } else if (isdigit(cur)) {
        bool is_float = false;
        while (is_sym_char(cur = input.get())) {
            num_stream << cur;
            if (cur == '.')
                is_float = true;
        }
        input.putback(cur);
        
        Number *rval;
        if (is_float) {
            double num;
            num_stream >> num;
            rval = new /*(NoGC)*/ Float(num);
        } else {
            long num;
            num_stream >> num;
            rval = new /*(NoGC)*/ Int(num);
        }
                    
        if (!num_stream.eof())
            throw ReaderError("Invalid number format: ", num_stream.str());

        return rval;
    } else {
        input.putback(cur);
        if (sign) input.putback(sign);
        return read_symbol(input);
    }
}

Symbol *read_symbol(istream &input) {
    string sym;
    char cur = input.get();
    while(is_sym_char(cur)) {
        sym += cur;
        cur = input.get();
    } 
    input.putback(cur);
    return new /*(NoGC)*/ Symbol(sym);
}

Pair *read_list(istream &input) {
    char cur = killws(input);

    if (cur == ')')
        return (Pair*)NIL;
    
    input.putback(cur);
    Form *car = read_form(input);
    cur = killws(input);
    Form *cdr;
    if (cur == '.') {
        cdr = read_form(input);
        cur = killws(input);
        if (cur != ')')
            throw ReaderError("only one element may succeed '.' in an irregular list");
    } else {
        input.putback(cur);
        cdr = read_list(input);
    }

    return new /*(NoGC)*/ Pair(car, cdr);
}

Form *read_form(istream &input) {
    char cur = killws(input);

    if (isdigit(cur) || cur == '-' || cur == '+') {
        input.putback(cur);
        return read_number(input);
    }
    if (cur == '(')
        return read_list(input);
    if (cur == '\'') {
        Form *f = read_form(input);
        return cons(new /*(NoGC)*/ Symbol("quote"), cons(f, NIL));
    } if (is_sym_char(cur)) {
        input.putback(cur);
        return read_symbol(input);
    }

    input.putback(cur);
    string extra;
    input >> extra;
    throw ReaderError(string("Extraneous input: ") + extra);
}
