#include <cstdio>
#include <sys/ptrace.h>
#include <unistd.h>
#include <AK/LogStream.h>
#include <sys/wait.h>
#include <LibC/sys/arch/i386/regs.h>
#include <Kernel/Syscall.h>

int main()
{
    dbg() << "hello";
    int pid = fork();
    if(!pid) {
        for(;;) {
            dbg() << "child\n";
            usleep(100 * 1000);
        }
    }

    sleep(1);

    printf("attaching\n");
    dbg() << "attaching";

    if (ptrace(PT_ATTACH, pid, 0, 0) == -1) {
        perror("attach");
        return 1;
    }
    
    dbg() << "attached";

    if (waitpid(pid, nullptr, WSTOPPED) != pid) {
        perror("waitpid");
        return 1;
    }

    dbg() << "continuing";

    for(;;) {
        if (ptrace(PT_SYSCALL, pid, 0, 0) == -1) {
            perror("syscall");
            return 1;
        }
        if (waitpid(pid, nullptr, WSTOPPED) != pid) {
            perror("waitpid");
            return 1;
        }

        PtraceRegisters regs = {};
        if (ptrace(PT_GETREGS, pid, &regs, 0) == -1) {
            perror("getregs");
            return 1;
        }

        dbg() << "syscall: " << Syscall::to_string(static_cast<Syscall::Function>(regs.eax));

        sleep(1);
    }
    sleep(5);

    return 0;
}
