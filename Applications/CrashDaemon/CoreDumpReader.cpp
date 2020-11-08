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

#include "CoreDumpReader.h"
#include <string.h>

OwnPtr<CoreDumpReader> CoreDumpReader::create(const LexicalPath& path)
{
    auto mapped_file = make<MappedFile>(path.string());
    if (!mapped_file->is_valid())
        return nullptr;
    return make<CoreDumpReader>(move(mapped_file));
}

CoreDumpReader::CoreDumpReader(OwnPtr<MappedFile>&& coredump_file)
    : m_coredump_file(move(coredump_file))
    , m_coredump_image((u8*)m_coredump_file->data(), m_coredump_file->size())
{
    size_t index = 0;
    m_coredump_image.for_each_program_header([this, &index](auto pheader) {
        dbgln("{:p}", pheader.vaddr().as_ptr());
        if (pheader.type() == PT_NOTE) {
            m_notes_segment_index = index;
        }
        ++index;
    });
    ASSERT(m_notes_segment_index != -1);
}

CoreDumpReader::~CoreDumpReader()
{
}

CoreDumpReader::NotesEntryIterator::NotesEntryIterator(const u8* notes_data)
    : m_current((const ELF::Core::NotesEntry*)notes_data)
    , start(notes_data)
{
}

ELF::Core::NotesEntry::Type CoreDumpReader::NotesEntryIterator::type() const
{
    ASSERT(m_current->type == ELF::Core::NotesEntry::Type::MemoryRegionInfo
        || m_current->type == ELF::Core::NotesEntry::Type::ThreadInfo
        || m_current->type == ELF::Core::NotesEntry::Type::Null);
    return m_current->type;
}

const ELF::Core::NotesEntry* CoreDumpReader::NotesEntryIterator::current() const
{
    return m_current;
}

void CoreDumpReader::NotesEntryIterator::next()
{
    ASSERT(!at_end());
    if (type() == ELF::Core::NotesEntry::Type::ThreadInfo) {
        m_current = (const ELF::Core::NotesEntry*)(((const u8*)m_current) + sizeof(ELF::Core::NotesEntry) + sizeof(ELF::Core::ThreadInfo));
        return;
    }
    if (type() == ELF::Core::NotesEntry::Type::MemoryRegionInfo) {
        const char* region_file_name_start = (const char*)(m_current) + sizeof(ELF::Core::NotesEntry) + sizeof(ELF::Core::MemoryRegionInfo);
        m_current = (const ELF::Core::NotesEntry*)(region_file_name_start + strlen(region_file_name_start) + 1);
        return;
    }
}

bool CoreDumpReader::NotesEntryIterator::at_end() const
{
    return type() == ELF::Core::NotesEntry::Type::Null;
}
