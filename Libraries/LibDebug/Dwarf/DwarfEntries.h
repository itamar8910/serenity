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
#include <AK/HashMap.h>
#include <AK/OwnPtr.h>
#include <AK/String.h>
#include <LibELF/Image.h>

namespace Dwarf {

enum class EntryTag : u32 {
    None = 0,
    SubProgram = 0x2e,
    Variable = 0x34,
};

enum class Attribute : u32 {
    None = 0,
    Name = 0x3,
    LowPc = 0x11,
    HighPc = 0x12,
};

struct AttributeValue {
    enum class Type {
        UnsignedNumber,
        SignedNumber,
        String,
        DieReference, // Reference to another DIE in the same compilation unit
        Boolean,
        DwarfExpression,
    } type;

    union {
        u32 as_u32;
        i32 as_i32;
        const char* as_string; // points to bytes in the elf image
        bool as_bool;
        struct {
            u32 length;
            const u8* bytes; // points to bytes in the elf image
        } as_dwarf_expression;
    } data;
};

struct Entry {
    EntryTag tag;
    u32 offset;
    bool has_children;
    HashMap<Attribute, AttributeValue> attributes;
};

enum class AttributeDataForm : u32 {
    None = 0,
    Addr = 0x1,
    Data2 = 0x5,
    Data4 = 0x6,
    String = 0x8,
    Data1 = 0xb,
    StringPointer = 0xe,
    Ref4 = 0x13,
    SecOffset = 0x17,
    ExprLoc = 0x18,
    FlagPresent = 0x19,
};

struct AttributeSpecification {
    Attribute attribute;
    AttributeDataForm form;
};

struct AbbreviationEntry {

    EntryTag tag;
    bool has_children;

    Vector<AttributeSpecification> attribute_specifications;
};

struct [[gnu::packed]] CompilationUnitHeader
{
    u32 length;
    u16 version;
    u32 abbrev_offset;
    u8 address_size;
};

class DebugEntries {
public:
    DebugEntries(const ELF::Image&);

    const Vector<Entry>& entries() { return m_entries; }

private:
    void parse_entries();

    HashMap<u32, AbbreviationEntry> parse_abbreviation_info(u32 offset);

    Vector<Entry> parse_entries_for_compilation_unit(BufferStream& debug_info_stream, u32 end_offset, HashMap<u32, AbbreviationEntry> abbreviation_info_map);
    AttributeValue get_attribute_value(AttributeDataForm form, BufferStream& debug_info_stream) const;

    const ELF::Image& m_elf;
    Vector<Entry> m_entries;
    const ELF::Image::Section m_debug_info_section;
};

}

namespace AK {

template<>
struct Traits<Dwarf::Attribute> : public GenericTraits<Dwarf::Attribute> {
    static unsigned hash(const Dwarf::Attribute& attribute)
    {
        return int_hash(static_cast<u32>(attribute));
    }
};
}
