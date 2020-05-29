/*
 * Copyright (c) 2019-2020, Andrew Kaster <andrewdkaster@gmail.com>,
 * Itamar S. <itamar8910@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// int foo(int x)
// {
//     //
//     if (x == -1)
//         foo(0);
//     return 1;
// }a

// int libfunc();

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <AK/LogStream.h>
#include <AK/String.h>
#include <LibELF/DynamicLoader.h>

volatile int g_x = 0;

int main(int argc, char** argv, char** envp)
{
    (void)argc;
    (void)argv;
    // if (argc != 2) {
    //     fprintf(stderr, "usage: Loader [executable]\n");
    //     exit(1);
    // }
    //
    //s
    dbg() << "Loader main";
    printf("Dynamic loader started\n");
    char* main_program_path_ptr = getenv("_MAIN_PROGRAM_PATH");
    ASSERT(main_program_path_ptr);
    String main_program_path(main_program_path_ptr);
    char* main_program_fd_str = getenv("_MAIN_PROGRAM_FD");
    ASSERT(main_program_fd_str);
    int main_program_fd = atoi(main_program_fd_str);
    lseek(main_program_fd, 0, SEEK_SET);
    dbg() << "main_program: " << main_program_path << ", fd: " << main_program_fd;

    printf("Loading main program\n");
    void* res = serenity_dlopen(main_program_fd, main_program_path.characters(), RTLD_LAZY | RTLD_GLOBAL);
    dbg() << "dlopen res: " << res;
    dbg() << dlerror();

    const ELF::DynamicLoader& main_program = *reinterpret_cast<ELF::DynamicLoader*>(res);
    auto entry_point = main_program.entry_point();
    dbg() << "entry point: " << entry_point;
    typedef int (*EntryFunction)(int, char**, char**);
    int retval = ((EntryFunction)(entry_point.get()))(argc, argv, envp);
    printf("Main program return value: %d\n", retval);

    // sleep(1000); // so we can inspect memory layout etc

    /// TODO: close main_program_fd (preferably with a ScopeGuard)

    return retval;
}
