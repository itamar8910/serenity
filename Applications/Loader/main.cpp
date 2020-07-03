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

#include <LibELF/AuxiliaryData.h>

#include "DynamicSection.h"
#include "Utils.h"

void load_object(const ELF::AuxiliaryData& aux_data)
{
    dbgprintf("entry point: %p\n", aux_data.entry_point);
    dbgprintf("program headers: %p\n", aux_data.program_headers);
    dbgprintf("num program headers: %p\n", aux_data.num_program_headers);
    dbgprintf("base address: %p\n", aux_data.base_address);

    Elf32_Addr dynamic_section_addr = 0;
    for (size_t i = 0; i < aux_data.num_program_headers; ++i) {
        Elf32_Phdr* phdr = &((Elf32_Phdr*)aux_data.program_headers)[i];
        dbgprintf("phdr: %p\n", phdr);
        dbgprintf("phdr type: %d\n", phdr->p_type);
        if (phdr->p_type == PT_DYNAMIC) {
            dynamic_section_addr = aux_data.base_address + phdr->p_offset;
        }
    }

    if (!dynamic_section_addr)
        exit(1);

    DynamicSection dynamic_section(aux_data.base_address, dynamic_section_addr);

    /*
    for each needed_lib:
        load_program(needed_lib)
        resolve_relocations();
        call_init_functions();
    */
}

int main(int x, char**)
{
    ELF::AuxiliaryData* aux_data = (ELF::AuxiliaryData*)x;
    load_object(*aux_data);

    exit(0);
    return 0;
}
