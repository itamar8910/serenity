#include <Kernel/Syscall.h>

void local_dbgputstr(const char* str, int len)
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

void exit(int code)
{
    constexpr unsigned int function = Kernel::SC_exit;
    unsigned int result;
    asm volatile("int $0x82"
                 : "=a"(result)
                 : "a"(function), "d"(code)
                 : "memory");
}

int main()
{
    const char str[] = "loader\n";
    local_dbgputstr(str, sizeof(str));
    exit(0);
    return 0;
}
