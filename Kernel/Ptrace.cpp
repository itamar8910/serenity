#include <Kernel/Process.h>
#include <Kernel/Ptrace.h>
#include <Kernel/Thread.h>
#include <Kernel/ThreadTracer.h>
#include <Kernel/VM/MemoryManager.h>
#include <Kernel/VM/ProcessPagingScope.h>

namespace Ptrace {

KResultOr<u32> handle_syscall(const Kernel::Syscall::SC_ptrace_params& params, Process& caller)
{
    if (params.request == PT_TRACE_ME) {
        if (Thread::current->tracer())
            return KResult(-EBUSY);

        caller.set_wait_for_tracer_at_next_execve(true);
        return KSuccess;
    }

    if (params.pid == caller.pid())
        return KResult(-EINVAL);

    Thread* peer = nullptr;
    {
        InterruptDisabler disabler;
        peer = Thread::from_tid(params.pid);
    }
    if (!peer)
        return KResult(-ESRCH);

    if (peer->process().uid() != caller.euid())
        return KResult(-EACCES);

    if (params.request == PT_ATTACH) {
        if (peer->tracer()) {
            return KResult(-EBUSY);
        }
        peer->start_tracing_from(caller.pid());
        if (peer->state() != Thread::State::Stopped && !(peer->has_blocker() && peer->blocker().is_reason_signal()))
            peer->send_signal(SIGSTOP, &caller);
        return KSuccess;
    }

    auto* tracer = peer->tracer();

    if (!tracer)
        return KResult(-EPERM);

    if (tracer->tracer_pid() != caller.pid())
        return KResult(-EBUSY);

    if (peer->state() == Thread::State::Running)
        return KResult(-EBUSY);

    switch (params.request) {
    case PT_CONTINUE:
        peer->send_signal(SIGCONT, &caller);
        break;

    case PT_DETACH:
        peer->stop_tracing();
        peer->send_signal(SIGCONT, &caller);
        break;

    case PT_SYSCALL:
        tracer->set_trace_syscalls(true);
        peer->send_signal(SIGCONT, &caller);
        break;

    case PT_GETREGS: {
        if (!tracer->has_regs())
            return KResult(-EINVAL);

        PtraceRegisters* regs = reinterpret_cast<PtraceRegisters*>(params.addr);
        if (!caller.validate_write(regs, sizeof(PtraceRegisters)))
            return KResult(-EFAULT);

        {
            SmapDisabler disabler;
            *regs = tracer->regs();
        }
        break;
    }

    case PT_SETREGS: {
        if (!tracer->has_regs())
            return KResult(-EINVAL);

        if (!caller.validate_read(params.addr, sizeof(PtraceRegisters)))
            return KResult(-EFAULT);

        {
            SmapDisabler disabler;
            PtraceRegisters* regs = reinterpret_cast<PtraceRegisters*>(params.addr);
            RegisterState* peer_saved_registers = reinterpret_cast<RegisterState*>(peer->kernel_stack_top() - sizeof(RegisterState));
            copy_ptrace_registers_into_kernel_registers(*peer_saved_registers, *regs);
            break;
        }
    }

    case PT_PEEK: {
        u32* addr = reinterpret_cast<u32*>(params.addr);
        return peer->process().peek_user_data(addr);
    }

    case PT_POKE: {
        u32* addr = reinterpret_cast<u32*>(params.addr);
        return peer->process().poke_user_data(addr, params.data);
    }

    default:
        return -EINVAL;
    }

    return 0;
}

void copy_kernel_registers_into_ptrace_registers(PtraceRegisters& ptrace_regs, const RegisterState& kernel_regs)
{
    ptrace_regs.eax = kernel_regs.eax,
    ptrace_regs.ecx = kernel_regs.ecx,
    ptrace_regs.edx = kernel_regs.edx,
    ptrace_regs.ebx = kernel_regs.ebx,
    ptrace_regs.esp = kernel_regs.esp,
    ptrace_regs.ebp = kernel_regs.ebp,
    ptrace_regs.esi = kernel_regs.esi,
    ptrace_regs.edi = kernel_regs.edi,
    ptrace_regs.eip = kernel_regs.eip,
    ptrace_regs.eflags = kernel_regs.eflags,
    ptrace_regs.cs = 0;
    ptrace_regs.ss = 0;
    ptrace_regs.ds = 0;
    ptrace_regs.es = 0;
    ptrace_regs.fs = 0;
    ptrace_regs.gs = 0;
}

void copy_ptrace_registers_into_kernel_registers(RegisterState& kernel_regs, const PtraceRegisters& ptrace_regs)
{
    kernel_regs.eax = ptrace_regs.eax;
    kernel_regs.ecx = ptrace_regs.ecx;
    kernel_regs.edx = ptrace_regs.edx;
    kernel_regs.ebx = ptrace_regs.ebx;
    kernel_regs.esp = ptrace_regs.esp;
    kernel_regs.ebp = ptrace_regs.ebp;
    kernel_regs.esi = ptrace_regs.esi;
    kernel_regs.edi = ptrace_regs.edi;
    kernel_regs.eip = ptrace_regs.eip;
    kernel_regs.eflags = ptrace_regs.eflags;
}

}
