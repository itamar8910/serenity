#include "Syscalls.h"

extern "C" {

[[noreturn]] void exit(int code)
{
    constexpr unsigned int function = Kernel::SC_exit;
    unsigned int result;
    asm volatile("int $0x82"
                 : "=a"(result)
                 : "a"(function), "d"(code)
                 : "memory");
    while (1) {
    }
}

void dbgputstr(const char* str, int len)
{
    constexpr unsigned int function = Kernel::SC_dbgputch;
    for (int i = 0; i < len; ++i) {
        unsigned int result;
        asm volatile("int $0x82"
                     : "=a"(result)
                     : "a"(function), "d"((u32)str[i])
                     : "memory");
    }
}

void dbgputc(char c)
{
    constexpr unsigned int function = Kernel::SC_dbgputch;
    unsigned int result;
    asm volatile("int $0x82"
                 : "=a"(result)
                 : "a"(function), "d"(c)
                 : "memory");
}
}
