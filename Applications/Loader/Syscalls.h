#pragma once

#include <AK/Types.h>
#include <Kernel/Syscall.h>
#include <stdarg.h>

extern "C" {
void exit(int code);
void dbgputstr(const char* str, int len);
void dbgputc(char c);
}
