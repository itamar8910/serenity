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

#pragma once

#include <AK/LexicalPath.h>
#include <AK/MappedFile.h>
#include <AK/Noncopyable.h>
#include <AK/OwnPtr.h>
#include <LibELF/CoreDump.h>
#include <LibELF/Image.h>

class CoreDumpReader {
    AK_MAKE_NONCOPYABLE(CoreDumpReader);

public:
    static OwnPtr<CoreDumpReader> create(const LexicalPath&);
    ~CoreDumpReader();

    CoreDumpReader(OwnPtr<MappedFile>&&);

    template<typename Func>
    void for_each_memory_region_info(Func func);

private:
    class NotesEntryIterator {
    public:
        NotesEntryIterator(const u8* notes_data);

        ELF::Core::NotesEntryHeader::Type type() const;
        const ELF::Core::NotesEntry* current() const;

        void next();
        bool at_end() const;

    private:
        const ELF::Core::NotesEntry* m_current { nullptr };
        const u8* start { nullptr };
    };

    OwnPtr<MappedFile> m_coredump_file;
    ELF::Image m_coredump_image;
    ssize_t m_notes_segment_index { -1 };
};

template<typename Func>
void CoreDumpReader::for_each_memory_region_info(Func func)
{
    for (NotesEntryIterator it((const u8*)m_coredump_image.program_header(m_notes_segment_index).raw_data()); !it.at_end(); it.next()) {
        if (it.type() != ELF::Core::NotesEntryHeader::Type::MemoryRegionInfo)
            continue;
        auto* region = (const ELF::Core::MemoryRegionInfo*)(it.current());
        IterationDecision decision = func(region);
        if (decision == IterationDecision::Break)
            return;
    }
}
