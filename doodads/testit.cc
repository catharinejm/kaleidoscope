// #include <iostream>
// #include <memory>
// #include <string>
// #include <sstream>


//#include <gc/gc_cpp.h>

// using namespace std;

class A {
protected:
    int x;
public:
    A(int i): x(i) {}
    virtual int add(int y) { return x + y; }
};

class B : public A {
public:
    B(int i) : A(i) {}
    int add(int y) { return x + y; }
    int sub(int y) { return x - y; }
};

int func(B &a) {
    a.sub(10);
    return a.add(40);
}

int main(int argc, char *argv[]) {
    //GC_INIT()

    B b(10);
    // A a = b;
    // a.add(10);

    // cerr << "sizeof(void*): " << sizeof(void*) << endl;
    // auto p = &A::add;
    // cerr << "sizeof(&A::add): " << sizeof(&A::add) << endl;
    // cerr << "sizeof(&p): " << sizeof(&p) << endl;
    // cerr << "sizeof(&B::sub): " << sizeof(&B::sub) << endl;
    // cerr << "sizeof(&func): " << sizeof(&func) << endl;

    return 0;
}
