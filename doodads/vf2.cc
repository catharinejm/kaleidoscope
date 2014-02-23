#include <string>
#include <iostream>
#include <dlfcn.h>

using namespace std;

template<typename T>
static void* voidify(T method)
{
    asm ("movq %rdi, %rax"); // should work on x86_64 ABI compliant platforms
}

template<typename T>
const char* getMethodName(T method)
{
    Dl_info info;
    if (dladdr(voidify(method), &info))
        return info.dli_sname;
    return "";
}

class Foo {
public:
    Foo() {}

    void bar() {}
};

int main() {
    cout << getMethodName(&Foo::bar) << endl;
    
    return 0;
}
