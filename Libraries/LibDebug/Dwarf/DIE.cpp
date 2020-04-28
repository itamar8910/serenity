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

#include "DIE.h"
#include "CompilationUnit.h"
#include "DwarfInfo.h"
#include <AK/BufferStream.h>
#include <AK/ByteBuffer.h>

namespace Dwarf {

DIE::DIE(const CompilationUnit& unit, u32 offset)
    : m_compilation_unit(unit)
    , m_offset(offset)
{
    BufferStream stream(const_cast<ByteBuffer&>(m_compilation_unit.dwarf_info().debug_info_data()));
    stream.advance(m_offset);
    stream.read_LEB128_unsigned(m_abbreviation_code);
    if (m_abbreviation_code == 0) {
        m_tag = EntryTag::None;
    } else {
        auto abbreviation_info = m_compilation_unit.abbreviations_map().get(m_abbreviation_code);
        ASSERT(abbreviation_info.has_value());
        m_tag = abbreviation_info.value().tag;
        m_has_children = abbreviation_info.value().has_children;
        for (auto attribute_spec : abbreviation_info.value().attribute_specifications) {
            get_attribute_value(attribute_spec.form, stream);
        }
    }
    m_size = stream.offset() - m_offset;
}

DIE::AttributeValue DIE::get_attribute_value(AttributeDataForm form,
    BufferStream& debug_info_stream) const
{
    AttributeValue value;
    switch (form) {
    case AttributeDataForm::StringPointer: {
        auto strings_data = m_compilation_unit.dwarf_info().debug_strings_data();
        u32 offset = 0;
        debug_info_stream >> offset;
        value.type = AttributeValue::Type::String;
        value.data.as_string = reinterpret_cast<const char*>(strings_data.data() + offset);
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
        value.data.as_dwarf_expression.bytes = reinterpret_cast<const u8*>(m_compilation_unit.dwarf_info().debug_info_data().data() + debug_info_stream.offset());
        debug_info_stream.advance(length);
        break;
    }
    case AttributeDataForm::String: {
        String str;
        u32 str_offset = debug_info_stream.offset();
        debug_info_stream >> str;
        value.type = AttributeValue::Type::String;
        value.data.as_string = reinterpret_cast<const char*>(str_offset + m_compilation_unit.dwarf_info().debug_info_data().data());
        break;
    }
    default:
        dbg() << "Unimplemented AttributeDataForm: " << (u32)form;
        ASSERT_NOT_REACHED();
    }
    return value;
}

}
