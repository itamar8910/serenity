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
#include <AK/ByteBuffer.h>
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

class AbbreviationInfo : public RefCounted<AbbreviationInfo> {

public:
    static NonnullRefPtr<AbbreviationInfo> create(const ELF::Image& elf, u32 offset)
    {
        return adopt(*new AbbreviationInfo(elf, offset));
    }

    Optional<AbbreviationEntry> from_code(u32 code) const;

private:
    AbbreviationInfo(const ELF::Image& elf, u32 offset);

    HashMap<u32, AbbreviationEntry> m_entries;
};

class Entry {
public:
    Entry(const ELF::Image& elf, u32 offset, NonnullRefPtr<AbbreviationInfo>);

    Optional<AttributeValue> get_attribute(const Attribute&) const;

    bool is_null() const { return m_abbreviation_code == 0; }
    u32 abbreviation_code() const { return m_abbreviation_code; }
    size_t size() const { return m_size; };
    EntryTag tag() const { return m_tag; };
    u32 offset() const { return m_offset; };

    AbbreviationEntry abbreviation_info_of_entry() const;

    static void update_debug_info(const ByteBuffer&);

private:
    static ByteBuffer s_cached_debug_info;

    static const ByteBuffer& cached_debug_info();

    AttributeValue get_attribute_value(AttributeDataForm form, BufferStream& debug_info_stream) const;

    u32 m_offset { 0 };
    u32 m_data_offset { 0 };
    size_t m_abbreviation_code { 0 };
    EntryTag m_tag { EntryTag::None };
    bool m_has_children { false };
    size_t m_size { 0 };
    const ELF::Image& m_elf;
    NonnullRefPtr<AbbreviationInfo> m_abbreviation_info;

    // HashMap<Attribute, AttributeValue> attributes;
};

class DebugEntries {
public:
    DebugEntries(const ELF::Image&);

    const Vector<Entry>& entries() { return m_entries; }

private:
    void parse_entries();

    void parse_entries_for_compilation_unit(ByteBuffer& debug_info,
        u32 compilation_unit_offset,
        u32 compilation_unit_length,
        NonnullRefPtr<AbbreviationInfo>& abbreviation_info);

    const ELF::Image& m_elf;
    Vector<Entry> m_entries;
    const ELF::Image::Section m_debug_info_section;
};

}
