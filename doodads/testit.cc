#include <iostream>
#include <memory>
#include <string>
#include <sstream>


//#include <gc/gc_cpp.h>

using namespace std;

struct A {
    string name;
    A() : name("bobo") {}// cerr << "A ctor" << endl; }
    ~A() {}// cerr << "~A dtor" << endl; }
};

struct B {
    shared_ptr<A> _a;
    B() : _a(new A) {}// cerr << "B ctor" << endl; }
    ~B() {}// cerr << "~B dtor" << endl; }
};

struct C {
    shared_ptr<B> _b;
    C() : _b(new B) {}// cerr << "C ctor" << endl; }
    ~C() {}// cerr << "~C dtor" << endl; }
};

shared_ptr<C> get_shared() { return shared_ptr<C>(new C); }

int main(int argc, char *argv[]) {
    // GC_INIT();

    if (argc < 2){
        cerr << "Usage: " << argv[0] << " <iterations>" << endl;
        exit(1);
    }
    stringstream s;
    s << argv[1];
    long iters;
    s >> iters;

    for (int x = 0; x < iters; x++) {
        shared_ptr<C> c = get_shared();
        c.reset();
    }

    return 0;
}
