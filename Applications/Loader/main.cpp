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

#include "DynamicObject.h"
#include "LibC/limits.h"
#include "Syscalls.h"
#include "Utils.h"

#define ALIGN_ROUND_UP(x, align) ((((size_t)(x)) + align - 1) & (~(align - 1)))

uint8_t loaded_libs_buffer[sizeof(List<DynamicObject>)];
List<DynamicObject>* loaded_libs_list = nullptr;

bool is_library_loaded(const char* lib_name)
{
    for (const auto loaded_lib : *loaded_libs_list) {
        if (!strcmp(lib_name, loaded_lib.object_name())) {
            return true;
        }
    }
    return false;
}

ELF::AuxiliaryData load_library(int fd, const char* library_name, const Elf32_Ehdr* elf_header)
{
    ASSERT(elf_header->e_type == ET_DYN);
    const uint8_t* image_base = (const uint8_t*)elf_header;

    const Elf32_Phdr* text_header = nullptr;
    const Elf32_Phdr* data_header = nullptr;
    const Elf32_Phdr* dynamic_header = nullptr;

    for (size_t program_header_index = 0; program_header_index < elf_header->e_phnum; ++program_header_index) {
        const Elf32_Phdr* program_header = ((const Elf32_Phdr*)(image_base + elf_header->e_phoff)) + program_header_index;
        if (program_header->p_type == PT_LOAD) {
            if (program_header->p_flags & PF_X) {
                text_header = program_header;
            } else {
                ASSERT(program_header->p_flags & (PF_R | PF_W));
            }
            data_header = program_header;
        } else if (program_header->p_type == PT_DYNAMIC) {
            dynamic_header = program_header;
        }
    }

    ASSERT(text_header);
    ASSERT(data_header);
    ASSERT(dynamic_header);

    char text_section_name[256];
    // TODO: snprintf
    sprintf(text_section_name, "%s - text", library_name);

    void* base_address = serenity_mmap(
        nullptr,
        text_header->p_memsz,
        PROT_READ | PROT_EXEC,
        MAP_SHARED, // TODO: Is this right?
        fd,
        text_header->p_offset,
        text_header->p_align,
        text_section_name);

    ASSERT(base_address);

    dbgprintf("text[0] = 0x%x\n", *(uint32_t*)base_address);

    u32 text_segment_size = ALIGN_ROUND_UP(text_header->p_memsz, text_header->p_align);

    // TODO: use this
    // void* dynamic_section_address = dynamic_header.value().vaddr().offset((u32)m_base_address).as_ptr();

    char data_section_name[256];
    // TODO: snprintf
    sprintf(data_section_name, "%s - data", library_name);

    void* data_segment_begin = serenity_mmap(
        (void*)((u32)base_address + text_segment_size),
        data_header->p_memsz,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        0, // no fd
        0, // 0 offset
        data_header->p_align,
        data_section_name);

    ASSERT(data_segment_begin);

    uint8_t* data_segment_actual_addr = (uint8_t*)(data_header->p_vaddr + ((u32)base_address));
    memcpy(data_segment_actual_addr, (const u8*)elf_header + data_header->p_offset, data_header->p_filesz);
    dbgprintf("data[0] = 0x%x\n", *(uint32_t*)data_segment_actual_addr);

    ELF::AuxiliaryData aux;
    aux.program_headers = (u32)base_address + elf_header->e_phoff;
    aux.num_program_headers = elf_header->e_phnum;
    aux.entry_point = elf_header->e_entry + (u32)base_address;
    aux.base_address = (u32)base_address;
    return aux;
}

ELF::AuxiliaryData load_library(const char* library_name)
{
    constexpr size_t MAX_LIBNAME = 256;
    char library_path[MAX_LIBNAME] = "/usr/lib/";
    // TODO: snprintf
    sprintf(library_path, "/usr/lib/%s", library_name);

    dbgprintf("loading: %s\n", library_path);
    int fd = open(library_path, O_RDONLY);
    ASSERT(fd >= 0);
    struct stat lib_stat;
    int rc = fstat(fd, &lib_stat);
    ASSERT(!rc);

    void* mmap_result = serenity_mmap(nullptr, lib_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0, PAGE_SIZE, library_path);
    ASSERT(mmap_result != nullptr);

    const Elf32_Ehdr* elf_header = (Elf32_Ehdr*)(mmap_result);

    auto result = load_library(fd, library_name, elf_header);

    munmap(mmap_result, lib_stat.st_size);
    close(fd);

    return result;
}

void handle_loaded_object(const ELF::AuxiliaryData& aux_data)
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

    DynamicObject dynamic_object(aux_data);

    for (const auto needed_library : dynamic_object.needed_libraries()) {

        if (!is_library_loaded(needed_library)) {
            auto dependency_aux_data = load_library(needed_library);
            handle_loaded_object(dependency_aux_data);
        }
    }

    loaded_libs_list->append(dynamic_object);

    dbgprintf("object: %s\n", dynamic_object.object_name());
    dynamic_object.for_each_symbol([](DynamicObject::Symbol s) {
        dbgprintf("symbol: %s. defined? %d\n", s.name(), s.is_undefined() == false);
    });

    /*
    TODO:
    resolve_relocations();
    call_init_functions();
    */
}

int main(int x, char**)
{
    new (loaded_libs_buffer)(List<char*>)();
    loaded_libs_list = reinterpret_cast<List<DynamicObject>*>(loaded_libs_buffer);
    ELF::AuxiliaryData* aux_data = (ELF::AuxiliaryData*)x;
    handle_loaded_object(*aux_data);
    hang();

    exit(0);
    return 0;
}
