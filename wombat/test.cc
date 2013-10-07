#include <iostream>
#include <string>
#include <set>
#include <utility>
#include <climits>

using namespace std;

enum WTYPE { NIL_TYPE=0, DOUBLE, SYMBOL, CONS, POINTER };

union WVAL {
    double d;
    void *p;
};

class WVal {
public:
    WVAL val;
    WTYPE ty;
    
    WVal() {}
    WVal(WTYPE t) : ty(t) {}
    WVal(const WVal &other) : val(other.val) {}
};

class Number : public WVal {
public:
    Number(double d) : WVal(DOUBLE) {
        val.d = d;
    }
};

const WVal NIL(NIL_TYPE);

class Cons : public WVal {
public:
    WVal head, tail;
    Cons(WVal hd, WVal tl) : WVal(CONS), head(hd), tail(tl) {
        val.p = (void*)this;
    }
    Cons(const WVal &other) : WVal(other) {
        Cons o = static_cast<Cons>(other);
        head = o.head;
        tail = o.tail;
    }
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

WVal build_cons(WVal h, WVal t) {
    return Cons(h, t);
}


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

    Number n(10);
    Cons c(build_cons(n, NIL));

    cout << "c.head.val: " << c.head.val.d << endl;
            
    return 0;
}
