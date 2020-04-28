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

#include "DwarfEntries.h"
#include <AK/BufferStream.h>
#include <AK/ByteBuffer.h>

namespace Dwarf {

ByteBuffer Entry::s_cached_debug_info {};

const ByteBuffer& Entry::cached_debug_info()
{
    return s_cached_debug_info;
}

void Entry::update_debug_info(const ByteBuffer& debug_info)
{
    s_cached_debug_info = ByteBuffer::wrap(debug_info.data(), debug_info.size());
}

AttributeValue Entry::get_attribute_value(AttributeDataForm form, BufferStream& debug_info_stream) const
{
    // dbg() << "get_attribute_value with form: " << (void*)form;
    AttributeValue value;
    switch (form) {
    case AttributeDataForm::StringPointer: {
        auto strings_section = m_elf.lookup_section(".debug_str");
        ASSERT(!strings_section.is_undefined());
        u32 offset = 0;
        debug_info_stream >> offset;
        value.type = AttributeValue::Type::String;
        value.data.as_string = reinterpret_cast<const char*>(strings_section.raw_data() + offset);
        break;
    }
    case AttributeDataForm::Data1: {
        u8 data = 0;
        debug_info_stream >> data;
        value.type = AttributeValue::Type::UnsignedNumber;
        value.data.as_u32 = data;
        break;
    }
    case AttributeDataForm::Data2: {
        u16 data = 0;
        debug_info_stream >> data;
        value.type = AttributeValue::Type::UnsignedNumber;
        value.data.as_u32 = data;
        break;
    }
    case AttributeDataForm::Addr: {
        u32 address = 0;
        debug_info_stream >> address;
        value.type = AttributeValue::Type::UnsignedNumber;
        value.data.as_u32 = address;
        break;
    }
    case AttributeDataForm::SecOffset:
        [[fallthrough]];
    case AttributeDataForm::Data4: {
        u32 data = 0;
        debug_info_stream >> data;
        value.type = AttributeValue::Type::UnsignedNumber;
        value.data.as_u32 = data;
        break;
    }
    case AttributeDataForm::Ref4: {
        u32 data = 0;
        debug_info_stream >> data;
        value.type = AttributeValue::Type::DieReference;
        value.data.as_u32 = data;
        break;
    }
    case AttributeDataForm::FlagPresent: {
        value.type = AttributeValue::Type::Boolean;
        value.data.as_bool = true;
        break;
    }
    case AttributeDataForm::ExprLoc: {
        size_t length = 0;
        debug_info_stream.read_LEB128_unsigned(length);
        value.type = AttributeValue::Type::DwarfExpression;
        value.data.as_dwarf_expression.length = length;
        value.data.as_dwarf_expression.bytes = reinterpret_cast<const u8*>(cached_debug_info().data() + debug_info_stream.offset());
        debug_info_stream.advance(length);
        break;
    }
    case AttributeDataForm::String: {
        String str;
        u32 str_offset = debug_info_stream.offset();
        debug_info_stream >> str;
        value.type = AttributeValue::Type::String;
        value.data.as_string = reinterpret_cast<const char*>(str_offset + cached_debug_info().data());
        break;
    }
    default:
        dbg() << "Unimplemented AttributeDataForm: " << (u32)form;
        ASSERT_NOT_REACHED();
    }
    return value;
}

void DebugEntries::parse_entries_for_compilation_unit(ByteBuffer& debug_info,
    u32 compilation_unit_offset,
    u32 compilation_unit_length,
    NonnullRefPtr<AbbreviationInfo>& abbreviation_info)
{

    BufferStream stream(debug_info);
    stream.advance(compilation_unit_offset);
    while ((u32)stream.offset() < compilation_unit_offset + compilation_unit_length) {
        auto offset = stream.offset();
        Entry entry(m_elf, offset, abbreviation_info);
        dbg() << (void*)offset << ":" << entry.abbreviation_code();

        stream.advance(entry.size());

        m_entries.empend(move(entry));
    }
}

Entry::Entry(const ELF::Image& elf, u32 offset, NonnullRefPtr<AbbreviationInfo> abbreviation_info)
    : m_offset(offset)
    , m_elf(elf)
    , m_abbreviation_info(abbreviation_info)
{

    // We have to const_cast here because there isn't a version of
    // BufferStream that accepts a non-const ByteStream
    // We take care not to use BufferStream operations that modify the underlying buffer
    // TOOD: We could add to AK a variant of BufferStream that operates on a const ByteBuffer
    BufferStream stream(const_cast<ByteBuffer&>(cached_debug_info()));
    stream.advance(offset);
    stream.read_LEB128_unsigned(m_abbreviation_code);
    m_data_offset = stream.offset();
    if (m_abbreviation_code == 0) {
        // An abbrevation code of 0 ( = null DIE entry) means the end of a chain of sibilings
        m_tag = EntryTag::None;
    } else {
        auto info = abbreviation_info_of_entry();
        m_tag = info.tag;
        m_has_children = info.has_children;

        for (auto attribute_spec : info.attribute_specifications) {
            get_attribute_value(attribute_spec.form, stream);
        }
    }
    m_size = stream.offset() - m_offset;
}

Optional<AttributeValue> Entry::get_attribute(const Attribute& attribute) const
{
    BufferStream stream(const_cast<ByteBuffer&>(cached_debug_info()));
    stream.advance(m_data_offset);
    auto info = abbreviation_info_of_entry();
    for (const auto& attribute_spec : info.attribute_specifications) {
        auto value = get_attribute_value(attribute_spec.form, stream);
        if (attribute_spec.attribute == attribute) {
            return value;
        }
    }
    return {};
}

AbbreviationEntry Entry::abbreviation_info_of_entry() const
{
    // auto val = m_abbreviation_info->from_code((u32)m_abbreviation_code);
    auto info = m_abbreviation_info->from_code((u32)m_abbreviation_code);
    ASSERT(info.has_value());
    return info.value();
}

DebugEntries::DebugEntries(const ELF::Image& image)
    : m_elf(image)
    , m_debug_info_section(image.lookup_section(".debug_info"))
{
    ASSERT(!m_debug_info_section.is_undefined());
    parse_entries();
}

void DebugEntries::parse_entries()
{
    auto debug_info_buffer = ByteBuffer::wrap(reinterpret_cast<const u8*>(m_debug_info_section.raw_data()), m_debug_info_section.size());
    Entry::update_debug_info(debug_info_buffer);

    BufferStream debug_info_stream(debug_info_buffer);

    while (!debug_info_stream.at_end()) {
        dbg() << "Start of compilation unit, offset: " << (void*)debug_info_stream.offset();
        CompilationUnitHeader compilation_unit_header {};
        debug_info_stream.read_raw(reinterpret_cast<u8*>(&compilation_unit_header), sizeof(CompilationUnitHeader));
        dbg() << compilation_unit_header.address_size;
        ASSERT(compilation_unit_header.address_size == sizeof(u32));
        ASSERT(compilation_unit_header.version == 4);
        auto abbreviation_info = AbbreviationInfo::create(m_elf, compilation_unit_header.abbrev_offset);
        // auto abbreviation_info = parse_abbreviation_info(compilation_unit_header.abbrev_offset);
        u32 length_after_header = compilation_unit_header.length - (sizeof(CompilationUnitHeader) - offsetof(CompilationUnitHeader, version));
        // u32 length_after_header = compilation_unit_header.length - offsetof(CompilationUnitHeader, length) - 1;
        parse_entries_for_compilation_unit(debug_info_buffer, debug_info_stream.offset(), length_after_header, abbreviation_info);
        debug_info_stream.advance(length_after_header);
    }
}

AbbreviationInfo::AbbreviationInfo(const ELF::Image& elf, u32 offset)
{
    auto abbreviation_section = elf.lookup_section(".debug_abbrev");
    ASSERT(!abbreviation_section.is_undefined());
    ASSERT(offset < abbreviation_section.size());

    auto abbreviation_buffer = ByteBuffer::wrap(reinterpret_cast<const u8*>(abbreviation_section.raw_data() + offset), abbreviation_section.size() - offset);
    BufferStream abbreviation_stream(abbreviation_buffer);

    while (!abbreviation_stream.at_end()) {
        size_t abbreviation_code = 0;
        abbreviation_stream.read_LEB128_unsigned(abbreviation_code);
        dbg() << "Abbrev code: " << abbreviation_code;
        // An abbrevation code of 0 marks the end of the
        // abbrevations for a given compilation unit
        if (abbreviation_code == 0)
            break;

        size_t tag {};
        abbreviation_stream.read_LEB128_unsigned(tag);
        u8 has_children = 0;
        abbreviation_stream >> has_children;

        AbbreviationEntry abbrevation_entry {};
        abbrevation_entry.tag = static_cast<EntryTag>(tag);
        abbrevation_entry.has_children = (has_children == 1);

        AttributeSpecification current_attribute_specification {};
        do {
            size_t attribute_value = 0;
            size_t form_value = 0;
            abbreviation_stream.read_LEB128_unsigned(attribute_value);
            abbreviation_stream.read_LEB128_unsigned(form_value);
            current_attribute_specification.attribute = static_cast<Attribute>(attribute_value);
            current_attribute_specification.form = static_cast<AttributeDataForm>(form_value);

            if (current_attribute_specification.attribute != Attribute::None) {
                abbrevation_entry.attribute_specifications.append(current_attribute_specification);
            }
        } while (current_attribute_specification.attribute != Attribute::None || current_attribute_specification.form != AttributeDataForm::None);

        m_entries.set((u32)abbreviation_code, abbrevation_entry);
    }
}

Optional<AbbreviationEntry> AbbreviationInfo::from_code(u32 code) const
{
    return m_entries.get(code);
}
}
