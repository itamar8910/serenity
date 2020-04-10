/*
 * Copyright (c) 2020, Itamar S. <itamar8910@gmail.com>
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

#include "DebugSession.h"
#include <AK/Optional.h>
#include <stdlib.h>

DebugSession::DebugSession(int pid)
    : m_debugee_pid(pid)
{
}

DebugSession::~DebugSession()
{
    if (!m_is_debugee_dead) {
        if (ptrace(PT_DETACH, m_debugee_pid, 0, 0) < 0) {
            perror("PT_DETACH");
        }
    }
}

OwnPtr<DebugSession> DebugSession::exec_and_attach(const String& file, const String& args)
{
    int pid = fork();

    if (!pid) {
        if (ptrace(PT_TRACE_ME, 0, 0, 0) < 0) {
            perror("PT_TRACE_ME");
            exit(1);
        }

        dbg() << "execl: " << file << ", " << args;
        int rc = execlp(file.characters(), args.characters());
        if (rc < 0) {
            perror("execvp");
        }
        ASSERT_NOT_REACHED();
    }

    if (waitpid(pid, nullptr, WSTOPPED) != pid) {
        perror("waitpid");
        return nullptr;
    }

    if (ptrace(PT_ATTACH, pid, 0, 0) < 0) {
        perror("PT_ATTACH");
        return nullptr;
    }

    // TOOD: do we actually need this waitpid?
    if (waitpid(pid, nullptr, WSTOPPED) != pid) {
        perror("waitpid");
        return nullptr;
    }

    if (ptrace(PT_CONTINUE, pid, 0, 0) < 0) {
        perror("continue");
        return nullptr;
    }

    // We want to continue until the exit from the 'execve' sycsall.
    // This ensures that when we start debugging the process
    // it executes the target image, and not the forked image of the tracing process.
    // NOTE: we only need to do this when we are debugging a new process (i.e not attaching to a process that's already running!)

    if (waitpid(pid, nullptr, WSTOPPED) != pid) {
        perror("wait_pid");
        return nullptr;
    }
    return make<DebugSession>(pid);
}

bool DebugSession::poke(u32* address, u32 data)
{
    if (ptrace(PT_POKE, m_debugee_pid, (void*)address, data) < 0) {
        perror("PT_POKE");
        return false;
    }
    return true;
}

Optional<u32> DebugSession::peek(u32* address) const
{
    Optional<u32> result;
    int rc = ptrace(PT_PEEK, m_debugee_pid, (void*)address, 0);
    if (errno == 0)
        result = static_cast<u32>(rc);
    return result;
}

bool DebugSession::insert_breakpoint(void* address)
{
    // We insert a software breakpoint by
    // patching the first byte of the instruction at 'address'
    // with the breakpoint instruction (int3)

    if (m_breakpoints.contains(address))
        return false;

    auto original_bytes = peek(reinterpret_cast<u32*>(address));

    if (!original_bytes.has_value())
        return false;

    if (!poke(reinterpret_cast<u32*>(address), (original_bytes.value() & ~(uint32_t)0xff) | BREAKPOINT_INSTRUCTION))
        return false;

    m_breakpoints.set(address, { address, original_bytes.value() });
    return true;
}

void DebugSession::remove_breakpoint(const BreakPoint& breakpoint)
{
    ASSERT(m_breakpoints.contains(breakpoint.address));
    poke(reinterpret_cast<u32*>(reinterpret_cast<char*>(breakpoint.address)), breakpoint.original_first_word);
    m_breakpoints.remove(breakpoint.address);
}

PtraceRegisters DebugSession::get_registers() const
{
    PtraceRegisters regs;
    if (ptrace(PT_GETREGS, m_debugee_pid, &regs, 0) < 0) {
        perror("PT_GETREGS");
        ASSERT_NOT_REACHED();
    }
    return regs;
}

void DebugSession::set_registers(const PtraceRegisters& regs)
{
    if (ptrace(PT_SETREGS, m_debugee_pid, reinterpret_cast<void*>(&const_cast<PtraceRegisters&>(regs)), 0) < 0) {
        perror("PT_SETREGS");
        ASSERT_NOT_REACHED();
    }
}

Optional<DebugSession::BreakPoint> DebugSession::get_matching_breakpoint(const PtraceRegisters& regs) const
{
    return m_breakpoints.get(reinterpret_cast<void*>(regs.eip - 1));
}

void DebugSession::continue_debugee()
{
    if (ptrace(PT_CONTINUE, m_debugee_pid, 0, 0) < 0) {
        perror("continue");
        ASSERT_NOT_REACHED();
    }
}
