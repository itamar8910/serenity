#include <AK/Format.h>

struct A {
    A()
    {
        dbgln("A()");
    }
    ~A()
    {
        dbgln("~A()");
    }
};

static A a;

int main()
{
}
