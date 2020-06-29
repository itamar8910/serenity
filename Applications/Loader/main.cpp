#include "Utils.h"
#include <LibELF/AuxiliaryData.h>

int main(int x, char**)
{
    ELF::AuxiliaryData* aux_data = (ELF::AuxiliaryData*)x;
    const char str[] = "loader\n";
    local_dbgputstr(str, sizeof(str));
    dbgprintf("hello %p\n", 0xdeadbeef);

    // Elf32_Phdr* phdr = (Elf32_Phdr*)aux_data->program_headers;
    // (void)ph
    // phdr->p_type
    // dbg() << "a";

    exit(aux_data->program_headers);
    return 0;
}
