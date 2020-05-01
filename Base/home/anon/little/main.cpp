#include <stdio.h>
#include <sys/stat.h>

int foo();

int main(int, char**)
{
    for (int i = 0; i < 3; ++i) {
        // This is a comment :^)
        printf("Hello friends!\n");
        mkdir("/tmp/xyz", 0755);
        printf("%d\n", foo());
    }
    return 0;
}

struct Foobar{
    int a;
    int b;
};

int foo(){
    int x = 1;
    char y = 2;
    x = x + (int)y;
    Foobar fb;
    fb.a = -2;
    fb.b = 6;
    return x + fb.a * fb.b;
}
