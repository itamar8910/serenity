/*
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
#include "List.h"
#include "Syscalls.h"
#include "Utils.h"
#include <LibELF/AuxiliaryData.h>
#include <LibELF/exec_elf.h>

class DynamicObject {
public:
    explicit DynamicObject(const ELF::AuxiliaryData&);
    ~DynamicObject() = default;

    List<const char*>& needed_libraries() { return m_needed_libraries; }
    const char* object_name() const
    {
        return m_object_name ? m_object_name : "[UNNAMED]";
    };

    class Symbol;

    Symbol symbol(size_t index) const;

    class Symbol {
    public:
        Symbol(const DynamicObject& dynamic, size_t index, const Elf32_Sym& sym)
            : m_dynamic(dynamic)
            , m_sym(sym)
            , m_index(index)
        {
        }

        static Symbol create_undefined(const DynamicObject& dynamic)
        {
            auto s = Symbol(dynamic, 0, {});
            s.m_is_undefined = true;
            return s;
        }

        ~Symbol() {}

        const char* name() const { return m_dynamic.symbol_string_table_string(m_sym.st_name); }
        unsigned section_index() const { return m_sym.st_shndx; }
        unsigned value() const { return m_sym.st_value; }
        unsigned size() const { return m_sym.st_size; }
        unsigned index() const { return m_index; }
        unsigned type() const { return ELF32_ST_TYPE(m_sym.st_info); }
        unsigned bind() const { return ELF32_ST_BIND(m_sym.st_info); }
        bool is_undefined() const { return (section_index() == 0) || m_is_undefined; }
        Elf32_Addr address() const { return value() + m_dynamic.base_address(); }

    private:
        const DynamicObject& m_dynamic;
        const Elf32_Sym& m_sym;
        const unsigned m_index;
        bool m_is_undefined { false };
    };

    template<typename Func>
    void for_each_symbol(Func f);

private:
    static Elf32_Addr find_dynamic_section_address(const ELF::AuxiliaryData&);
    void iterate_entries();

    const char* symbol_string_table_string(Elf32_Word index) const;
    Elf32_Addr base_address() const { return m_base_adderss; }

    Elf32_Addr m_base_adderss { 0 };
    const Elf32_Dyn* m_dynamic_section_entries { nullptr };

    Elf32_Addr m_string_table { 0 };
    Elf32_Addr m_hash_section { 0 };
    Elf32_Sym* m_dyn_sym_table { 0 };
    size_t m_symbol_count { 0 };

    List<const char*> m_needed_libraries;
    const char* m_object_name { nullptr };
};

template<typename Func>
void DynamicObject::for_each_symbol(Func f)
{
    for (size_t i = 0; i < m_symbol_count; ++i) {
        f(symbol(i));
    }
}
