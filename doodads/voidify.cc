#include <dlfcn.h>

#include <iostream>

using namespace std;

template<typename T>
const void* voidify(T method)
{
    void* pointer;
    asm ("movq %rdi, -8(%rbp)");
    return pointer;
}

template<typename T, typename M>
const char* getMethodName(const T* object, M method)
{
    Dl_info info;
    const void* methodPointer;
	
    if (sizeof method == 8) methodPointer = voidify(method); // actual method pointer
    else // vtable entry
    {
        struct VTableEntry
        {
            unsigned long long __pfn;
            unsigned long long __delta;
        };
        const VTableEntry* entry = static_cast<const VTableEntry*>(voidify(&method));
        const void* const* vtable = *(const void* const* const* const)object;
        methodPointer = vtable[(entry->__pfn - 1) / sizeof(void*)];
    }
	
    if (dladdr(voidify(methodPointer), &info))
        return info.dli_sname;
    return NULL;
}

class Foo {
public:
    Foo() {}

    void notVirtual() {}
    virtual void isVirtual() {}
};

int main() {
    Foo f;
    cout << getMethodName(&f, &Foo::notVirtual) << endl;// << getMethodName(&f, &Foo::isVirtual) << endl;
    
    return 0;
}
