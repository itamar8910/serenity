#include <cstdio>
#include <sys/ptrace.h>
#include <unistd.h>
#include <AK/LogStream.h>

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

    if(ptrace(PT_ATTACH, pid, 0, 0) == -1) {
        perror("attach");
        return 1;
    }
    
    // TODO: waitpid

    dbg() << "attached";

    sleep(2);

    dbg() << "continuing";

    if(ptrace(PT_CONTINUE, pid, 0, 0) == -1) {
        perror("continue");
        return 1;
    }

    // TODO: waitpid

    dbg() << "continued";

    sleep(5);

    return 0;
}
