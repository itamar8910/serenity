#include <cstdio>
#include <sys/ptrace.h>
#include <unistd.h>
#include <AK/LogStream.h>
#include <sys/wait.h>
#include <LibC/sys/arch/i386/regs.h>
#include <Kernel/Syscall.h>

int main()
{
    int pid = fork();
    if(!pid) {
        if (ptrace(PT_TRACE_ME, 0, 0, 0) == -1) {
            perror("traceme");
            return 1;
        }
        execl("/bin/ls", "ls", "/", nullptr);
    }

    if (waitpid(pid, nullptr, WSTOPPED) != pid) {
        perror("waitpid");
        return 1;
    }

    if (ptrace(PT_ATTACH, pid, 0, 0) == -1) {
        perror("attach");
        return 1;
    }
    
    if (waitpid(pid, nullptr, WSTOPPED) != pid) {
        perror("waitpid");
        return 1;
    }

    for(;;) {
        if (ptrace(PT_SYSCALL, pid, 0, 0) == -1) {
            if(errno == ESRCH)
                return 0;
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

        // skip syscall exit
        if (ptrace(PT_SYSCALL, pid, 0, 0) == -1) {
            if(errno == ESRCH)
                return 0;
            perror("syscall");
            return 1;
        }
        if (waitpid(pid, nullptr, WSTOPPED) != pid) {
            perror("waitpid");
            return 1;
        }

    }

    return 0;
}
