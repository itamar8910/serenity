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

#include "DebugInfo.h"
#include <AK/QuickSort.h>
#include <LibDebug/Dwarf/CompilationUnit.h>
#include <LibDebug/Dwarf/DwarfInfo.h>

DebugInfo::DebugInfo(NonnullRefPtr<const ELF::Loader> elf)
    : m_elf(elf)
{
    prepare_variable_scopes();
    prepare_lines();
}

void DebugInfo::prepare_variable_scopes()
{
    auto dwarf_info = Dwarf::DwarfInfo::create(m_elf);
    dwarf_info->for_each_compilation_unit([&](const Dwarf::CompilationUnit& unit) {
        // dbg() << "CU callback!: " << (void*)unit.offset();
        auto root = unit.root_die();
        parse_scopes_impl(root);
        // dbg() << "root die offset: " << (void*)root.offset();
        // root.for_each_child([&](const Dwarf::DIE& child) {
        //     if (child.tag() == Dwarf::EntryTag::SubProgram) {
        //         dbg() << "   child offset: " << (void*)child.offset();
        //         dbg() << "   Subprogram: " << (const char*)child.get_attribute(Dwarf::Attribute::Name).value().data.as_string;
        //     }
        // });
    });
}

void DebugInfo::parse_scopes_impl(const Dwarf::DIE& die)
{
    die.for_each_child([&](const Dwarf::DIE& child) {
        if (child.is_null())
            return;
        if (child.tag() == Dwarf::EntryTag::SubProgram || child.tag() == Dwarf::EntryTag::LexicalBlock) {
            if (child.get_attribute(Dwarf::Attribute::Inline).has_value()) {
                // We currently do not support inlined functions
                return;
            }
            if (child.get_attribute(Dwarf::Attribute::Ranges).has_value()) {
                // We currently do not support ranges
                return;
            }
            dbg() << "low pc: " << (void*)child.get_attribute(Dwarf::Attribute::LowPc).value().data.as_u32;
            dbg() << "high pc: " << (void*)child.get_attribute(Dwarf::Attribute::HighPc).value().data.as_u32;
            auto name = child.get_attribute(Dwarf::Attribute::Name);

            VariablesScope scope {};
            scope.is_function = (child.tag() == Dwarf::EntryTag::SubProgram);
            if (name.has_value())
                scope.name = name.value().data.as_string;
            dbg() << "scope name: " << (scope.name.has_value() ? scope.name.value() : "[Unnamed]");
            scope.address_low = child.get_attribute(Dwarf::Attribute::LowPc).value().data.as_u32;
            // Yes, the attribute name HighPc is confusing - it seems to actually be a positive offset from LowPc
            scope.address_high = scope.address_low + child.get_attribute(Dwarf::Attribute::HighPc).value().data.as_u32;
            child.for_each_child([&](const Dwarf::DIE& variable_entry) {
                if (variable_entry.tag() != Dwarf::EntryTag::Variable)
                    return;
                VariableInfo variable_info {};
                variable_info.name = variable_entry.get_attribute(Dwarf::Attribute::Name).value().data.as_string;
                dbg() << "variable: " << variable_info.name;
                scope.variables.append(variable_info);
            });
            m_scopes.append(scope);

            parse_scopes_impl(child);
        }
    });
}

void DebugInfo::prepare_lines()
{
    auto section = m_elf->image().lookup_section(".debug_line");
    ASSERT(!section.is_undefined());

    auto buffer = ByteBuffer::wrap(reinterpret_cast<const u8*>(section.raw_data()), section.size());
    BufferStream stream(buffer);

    Vector<LineProgram::LineInfo> all_lines;
    while (!stream.at_end()) {
        LineProgram program(stream);
        all_lines.append(program.lines());
    }

    for (auto& line_info : all_lines) {
        String file_path = line_info.file;
        if (file_path.contains("Toolchain/") || file_path.contains("libgcc"))
            continue;
        if (file_path.contains("serenity/")) {
            auto start_index = file_path.index_of("serenity/").value() + String("serenity/").length();
            file_path = file_path.substring(start_index, file_path.length() - start_index);
        }
        m_sorted_lines.append({ line_info.address, file_path, line_info.line });
    }
    quick_sort(m_sorted_lines, [](auto& a, auto& b) {
        return a.address < b.address;
    });
}

Optional<DebugInfo::SourcePosition> DebugInfo::get_source_position(u32 target_address) const
{
    if (m_sorted_lines.is_empty())
        return {};
    if (target_address < m_sorted_lines[0].address)
        return {};

    // TODO: We can do a binray search here
    for (size_t i = 0; i < m_sorted_lines.size() - 1; ++i) {
        if (m_sorted_lines[i + 1].address > target_address) {
            return Optional<SourcePosition>({ m_sorted_lines[i].file, m_sorted_lines[i].line, m_sorted_lines[i].address });
        }
    }
    return {};
}

Optional<u32> DebugInfo::get_instruction_from_source(const String& file, size_t line) const
{
    for (const auto& line_entry : m_sorted_lines) {
        if (line_entry.file == file && line_entry.line == line)
            return Optional<u32>(line_entry.address);
    }
    return {};
}

Optional<DebugInfo::VariablesScope> DebugInfo::get_scope_info(u32 address) const
{
    // TODO: We can store the scopes in a better data strucutre
    Optional<VariablesScope> best_matching_scope;

    for (const auto& scope : m_scopes) {
        if (scope.address_low <= address && scope.address_high > address) {

            if (!best_matching_scope.has_value()) {
                best_matching_scope = scope;

            } else if (scope.address_low > best_matching_scope.value().address_low || scope.address_high < best_matching_scope.value().address_high) {
                best_matching_scope = scope;
            }
        }
    }
    return best_matching_scope;
}
