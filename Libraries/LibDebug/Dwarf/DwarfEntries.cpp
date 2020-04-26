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

HashMap<u32, AbbreviationEntry> DebugEntries::parse_abbreviation_info(u32 offset)
{
    auto abbreviation_section = m_elf.lookup_section(".debug_abbrev");
    ASSERT(!abbreviation_section.is_undefined());
    ASSERT(offset < abbreviation_section.size());
    HashMap<u32, AbbreviationEntry> abbreviation_entries;

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

        abbreviation_entries.set((u32)abbreviation_code, abbrevation_entry);
    }

    return abbreviation_entries;
}

AttributeValue DebugEntries::get_attribute_value(AttributeDataForm form, BufferStream& debug_info_stream) const
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
        value.data.as_dwarf_expression.bytes = reinterpret_cast<const u8*>(m_debug_info_section.raw_data() + debug_info_stream.offset());
        debug_info_stream.advance(length);
        break;
    }
    case AttributeDataForm::String: {
        String str;
        u32 str_offset = debug_info_stream.offset();
        debug_info_stream >> str;
        value.type = AttributeValue::Type::String;
        value.data.as_string = reinterpret_cast<const char*>(str_offset);
        break;
    }
    default:
        dbg() << "Unimplemented AttributeDataForm: " << (u32)form;
        ASSERT_NOT_REACHED();
    }
    return value;
}

Vector<Entry> DebugEntries::parse_entries_for_compilation_unit(BufferStream& debug_info_stream, u32 end_offset, HashMap<u32, AbbreviationEntry> abbreviation_info_map)
{
    Vector<Entry> entries;
    while ((u32)debug_info_stream.offset() < end_offset) {
        auto offset = debug_info_stream.offset();
        dbg() << "     offset: " << (void*)offset << " / " << (void*)end_offset;
        size_t abbreviation_code = 0;
        debug_info_stream.read_LEB128_unsigned(abbreviation_code);
        if (abbreviation_code == 0) {
            dbg() << "null DIE entry";
            // An abbrevation code of 0 (A null DIE entry) means the end of a chain of sibilings
            entries.append({ EntryTag::None, 0, false, {} });
            continue;
        }

        dbg() << (void*)offset << ":" << abbreviation_code;

        auto entry_info = abbreviation_info_map.get(abbreviation_code);
        ASSERT(entry_info.has_value());

        Entry entry {};
        entry.tag = entry_info.value().tag;
        entry.offset = offset;
        entry.has_children = entry_info.value().has_children;

        // for (auto& spec : entry_info.value().attribute_specifications) {
        //     dbg() << "spec type: " << (void*)spec.attribute;
        //     dbg() << "spec form: " << (void*)spec.form;
        // }

        for (auto attribute_spec : entry_info.value().attribute_specifications) {
            auto value = get_attribute_value(attribute_spec.form, debug_info_stream);
            // dbg() << "value type: " << (u32)value.type;
            // dbg() << "value data: " << (u32)value.data.as_u32;
            entry.attributes.set(attribute_spec.attribute, value);
        }
    }
    return entries;
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
    BufferStream debug_info_stream(debug_info_buffer);

    while (!debug_info_stream.at_end()) {
        dbg() << "Start of compilation unit, offset: " << (void*)debug_info_stream.offset();
        CompilationUnitHeader compilation_unit_header {};
        debug_info_stream.read_raw(reinterpret_cast<u8*>(&compilation_unit_header), sizeof(CompilationUnitHeader));
        dbg() << compilation_unit_header.address_size;
        ASSERT(compilation_unit_header.address_size == sizeof(u32));
        ASSERT(compilation_unit_header.version == 4);
        auto abbreviation_info = parse_abbreviation_info(compilation_unit_header.abbrev_offset);
        u32 length_after_header = compilation_unit_header.length - (sizeof(CompilationUnitHeader) - offsetof(CompilationUnitHeader, version));
        // u32 length_after_header = compilation_unit_header.length - offsetof(CompilationUnitHeader, length) - 1;
        auto entries_of_unit = parse_entries_for_compilation_unit(debug_info_stream, debug_info_stream.offset() + length_after_header, abbreviation_info);
        m_entries.append(entries_of_unit);
    }
}

}
