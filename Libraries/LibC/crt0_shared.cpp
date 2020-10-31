#include <AK/Types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/internals.h>
#include <unistd.h>

extern "C" {

int main(int, char**, char**);

extern void __libc_init();
extern void _init();
extern char** environ;
extern bool __environ_is_malloced;

int _start(int argc, char** argv, char** env);
int _start(int argc, char** argv, char** env)
{
    _init();

    int status = main(argc, argv, env);
    // int status = main(argc, argv, environ);
    return status;
}
}

//void* __dso_handle = nullptr;

//void* __dso_handle __attribute__((visibility("hidden")));
void* __dso_handle __attribute__((__weak__));
