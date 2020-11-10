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

#include "Backtrace.h"

Backtrace::Backtrace(const LexicalPath& coredump_path)
    : m_coredump(CoreDumpReader::create(coredump_path))
{
    m_coredump->for_each_thread_info([this](const ELF::Core::ThreadInfo* thread_info) {
        dbgln("tid: {}, eip: {:p}", thread_info->tid, thread_info->regs.eip);

        uint32_t* ebp = (uint32_t*)thread_info->regs.ebp;
        dbgln("ebp: {}", ebp);
        uint32_t* eip = (uint32_t*)thread_info->regs.eip;
        while (ebp && eip) {
            auto* region = region_containing((FlatPtr)eip);
            if (region)
                dbgln("{:p}: {}", eip, (const char*)region->region_name);
            else
                dbgln("{:p}: ???", eip);

            auto next_eip = peek_memory((FlatPtr)(ebp + 1));
            auto next_ebp = peek_memory((FlatPtr)(ebp));
            if (!next_ebp.has_value() || !next_eip.has_value()) {
                dbgln("no value, exiting");
                break;
            }

            eip = (uint32_t*)next_eip.value();
            ebp = (uint32_t*)next_ebp.value();

            // dbgln("eip: {:p}, ebp: {:p}", eip, ebp);
        }

        return IterationDecision::Continue;
    });
}

Backtrace::~Backtrace()
{
}

Optional<uint32_t> Backtrace::peek_memory(FlatPtr address) const
{
    // dbgln("peek memory: {:p}", address);
    const auto* region = region_containing(address);
    if (!region)
        return {};

    FlatPtr offset_in_region = address - region->region_start;
    // dbgln("offset in region: {:x}, phdr index: {}", offset_in_region, region->program_header_index);
    const char* region_data = m_coredump->image().program_header(region->program_header_index).raw_data();
    return *(const uint32_t*)(&region_data[offset_in_region]);
}

const ELF::Core::MemoryRegionInfo* Backtrace::region_containing(FlatPtr address) const
{
    const ELF::Core::MemoryRegionInfo* ret = nullptr;
    m_coredump->for_each_memory_region_info([&ret, address](const ELF::Core::MemoryRegionInfo* region_info) {
        // dbgln("trying region: {}, start: {:p}, end: {:p}", (const char*)region_info->region_name, region_info->region_start, region_info->region_end);
        if (region_info->region_start <= address && region_info->region_end >= address) {
            ret = region_info;
            return IterationDecision::Break;
        }
        return IterationDecision::Continue;
    });
    return ret;
}
