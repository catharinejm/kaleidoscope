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

Number *read_number(istream &input) {
    char cur = input.get();
    bool is_neg = cur == '-';
    if (is_neg) cur = input.get();
    if (cur == '0') {
        if (input.eof())
            return new Int(0);
        else {
            cur = input.get();
            if (cur == '.') {
                string num_str("0.");
                while (is_sym_char(cur = input.get()))
                    num_str += cur;
                input.unget();
                double num;
                istringstream num_stream(num_str);
                num_stream >> num;
                if (!num_stream.eof())
                    throw ReaderError("Extraneous characters after number input");
                return new Float(is_neg ? -num : num);
            }

            long num;
            if (cur == 'x' || cur == 'X')
                input >> hex >> num;
            else if (isdigit(cur)) {
                input.unget();
                input >> oct >> num;
            }
            if (is_sym_char(input.get()))
                throw ReaderError("Extraneous characters after number input");
            else {
                input.unget();
                return new Int(is_neg ? -num : num);
            }
        }
    } else {
        string num_str;
        num_str += cur;
        bool is_float = false;
        while (is_sym_char(cur = input.get())) {
            if (cur == '.')
                is_float = true;
            num_str += cur;
        }
        input.unget();
        
        istringstream num_stream(num_str);
        if (is_float) {
            double num;
            num_stream >> num;
            
            if (!num_stream.eof())
                throw ReaderError("Extraneous characters after number input");

            return new Float(is_neg ? -num : num);
        } else {
            long num;
            num_stream >> num;

            if (!num_stream.eof())
                throw ReaderError("Extraneous characters after number input");
            
            return new Int(is_neg ? -num : num);
        }
    }
}

Symbol *read_symbol(istream &input) {
    string sym;
    char cur;
    do {
        cur = input.get();
        sym += cur;
    } while (is_sym_char(cur));
    input.unget();
    return new Symbol(sym);
}

Pair *read_list(istream &input) {
    char cur = killws(input);

    if (cur == ')')
        return (Pair*)NIL;
    
    input.unget();
    Form *car = read_form(input);
    cur = killws(input);
    Form *cdr;
    if (cur == '.') {
        cdr = read_form(input);
        cur = killws(input);
        if (cur != ')')
            throw ReaderError("only one element may succeed '.' in an irregular list");
    } else {
        input.unget();
        cdr = read_list(input);
    }

    return new Pair(car, cdr);
}

Form *read_form(istream &input) {
    char cur = killws(input);

    if (isdigit(cur) || cur == '-') {
        input.unget();
        return read_number(input);
    }
    if (cur == '(')
        return read_list(input);
    if (cur == '\'')
        return new Pair(new Symbol("quote"), read_form(input));
    if (is_sym_char(cur)) {
        input.unget();
        return read_symbol(input);
    }

    input.unget();
    string extra;
    input >> extra;
    throw ReaderError(string("Extraneous input: ") + extra);
}
