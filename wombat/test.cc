#include <iostream>
#include <string>
#include <set>
#include <utility>
#include <climits>

using namespace std;

union WVAL {
    double d;
    void *p;
};

class WVal {
public:
    WVAL val;
    
    WVal() {}
    WVal(WVAL v) : val(v) {}
    WVal(const WVal &other) : val(other.val) {}
};

class Symbol : public WVal {
    Symbol(const string *np) : WVal() {
        val.p = (void*)np;
    }
    Symbol(const string &n) : Symbol(&n) {}
public:
    static Symbol intern(const string &name) {
        static set<string> intern_set;
        auto rec = intern_set.insert(name);
        Symbol rval(*rec.first);
        return rval;
    }
    bool operator==(const Symbol &other) const { return val.p == other.val.p; }
    const string &name() const { return *(string*)val.p; }
};



int main() {
    Symbol a1 = Symbol::intern("a");
    Symbol a2 = Symbol::intern("a");
    Symbol b = Symbol::intern("b");

    cout << "a1 == a2: " << (a1 == a2) << endl
         << "a1 == b: " << (a1 == b) << endl;

    cout << "a1.name(): " << a1.name() << endl
         << "&a1.name(): " << &a1.name() << endl;
    cout << "a2.name(): " << a2.name() << endl
         << "&a2.name(): " << &a2.name() << endl;
    cout << "b.name(): " << b.name() << endl
         << "&b.name(): " << &b.name() << endl;

    return 0;
}
