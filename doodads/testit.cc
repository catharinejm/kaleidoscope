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
    virtual int add(int y) { return x + y; }
};

// int func(A &a) {
//     return a.add(40);
// }

int main(int argc, char *argv[]) {
    //GC_INIT()

    B b(10);
    A a(10);

    b.add(10);

    return 0;
}
