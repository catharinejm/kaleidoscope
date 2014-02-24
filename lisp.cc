#include "lisp.h"
#include "compiler.h"

#include <iostream>
#include <limits>

using namespace std;

string bleed_input(istream &input) {
    string rem;
    char rem_c[128];
    do {
        input.clear(ios::goodbit);
        input.getline(rem_c, streamsize(sizeof(rem_c)));
        rem += rem_c;
    } while (input.fail());
    return rem;
}

int main() {
    GC_INIT();
    InitializeNativeTarget();

    Compiler comp;

    for (;;) {
        try {
            cout << "> ";
            char c = cin.get();
            if (cin.eof()) exit(0);
            cin.putback(c);
            Form *f = read_form(cin);
            string leftovers = bleed_input(cin);
            if (leftovers.find_first_not_of(" \n\t") != string::npos)
                throw ReaderError(string("Extraneous characters after input: ") + leftovers);

            Function *stmt = comp.compile_top_level(f);
            stmt->dump();
            Form *res = ((Form*(*)())comp.get_fn_ptr(stmt))();
            cout << print_form(res) << endl;
        } catch (LispException e) {
            cerr << "ERROR: " << e.what() << endl;
        }
    } 
    
    return 0;
}
