#include "lisp.h"

#include <iostream>

using namespace std;

int main() {
    while(! cin.eof()) {
        try {
            cout << "> ";
            Form *f = read_form(cin);
            cout << print_form(f) << endl;
        } catch (LispException e) {
            cerr << "ERROR: " << e.what() << endl;
        }
    }
    
    return 0;
}
