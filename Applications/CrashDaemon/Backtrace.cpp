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

#include <AK/MappedFile.h>
#include <LibELF/Loader.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static String object_name(StringView memory_region_name)
{
    if (!memory_region_name.contains(":"))
        return {};
    if (memory_region_name.contains("Loader.so"))
        return "Loader.so";
    return memory_region_name.substring_view(0, memory_region_name.find_first_of(":").value()).to_string();
}

static String symbolicate(FlatPtr eip, const ELF::Core::MemoryRegionInfo* region)
{
    StringView region_name { region->region_name };

    auto name = object_name(region_name);

    String path;
    if (name.contains(".so"))
        path = String::format("/usr/lib/%s", name.characters());
    else {
        path = name;
    }

    struct stat st;
    if (stat(path.characters(), &st)) {
        return {};
    }

    auto mapped_file = make<MappedFile>(path);
    if (!mapped_file->is_valid())
        return {};

    auto loader = ELF::Loader::create((const u8*)mapped_file->data(), mapped_file->size());
    return loader->symbolicate(eip - region->region_start);
}

String Backtrace::backtrace_line(FlatPtr eip)
{
    auto* region = region_containing((FlatPtr)eip);
    if (!region) {
        return String::format("%p: ???", eip);
    }

    StringView region_name { region->region_name };
    if (region_name.contains("Loader.so"))
        return {};

    auto func_name = symbolicate(eip, region);

    return String::format("%p: [%s] %s", eip, object_name(region_name).characters(), func_name.is_null() ? "???" : func_name.characters());
}

Backtrace::Backtrace(const LexicalPath& coredump_path)
    : m_coredump(CoreDumpReader::create(coredump_path))
{
    size_t thread_index = 0;
    m_coredump->for_each_thread_info([this, &thread_index](const ELF::Core::ThreadInfo* thread_info) {
        dbgln("Backtrace for thread #{}, tid={}", thread_index++, thread_info->tid);

        uint32_t* ebp = (uint32_t*)thread_info->regs.ebp;
        uint32_t* eip = (uint32_t*)thread_info->regs.eip;
        while (ebp && eip) {

            auto line = backtrace_line((FlatPtr)eip);
            if (!line.is_null())
                dbgprintf("%s\n", line.characters());
            auto next_eip = peek_memory((FlatPtr)(ebp + 1));
            auto next_ebp = peek_memory((FlatPtr)(ebp));
            if (!next_ebp.has_value() || !next_eip.has_value()) {
                break;
            }

            eip = (uint32_t*)next_eip.value();
            ebp = (uint32_t*)next_ebp.value();
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
