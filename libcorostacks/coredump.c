#include <assert.h>

#include <libelf.h>
#include <string.h>

#include "libcorostacks.h"
#include "libcorostacks_int.h"

int
coredump_vmem_read(Elf *coredump_elf, csAddr_t addr, size_t nbytes, char *result)
{
    assert(coredump_elf);

    size_t phdr_num;
    if (elf_getphdrnum(coredump_elf, &phdr_num) < 0)
        goto elf_getphdrnum_fail;

    /* Find program header that contains desired addr,
     * and read the data */
    for (size_t i = 0; i < phdr_num; ++i)
    {
        GElf_Phdr phdr;
        GElf_Phdr *phdr_p = gelf_getphdr(coredump_elf, i, &phdr);
        if (!phdr_p || phdr.p_type != PT_LOAD)
            continue;

        GElf_Addr start = phdr.p_vaddr;
        GElf_Addr end = phdr.p_vaddr + phdr.p_memsz;
        
        if (addr < start || end <= addr)
            continue;

        if (addr + nbytes > end)
            goto size_too_big_fail;

        int64_t offset = phdr.p_offset + addr - start;
        Elf_Data *data = elf_getdata_rawchunk(coredump_elf, offset, nbytes, ELF_T_ADDR);
        if (data == NULL)
            goto elf_getdata_rawchunk_fail;

        assert(data->d_size == nbytes);

        memcpy(result, data->d_buf, nbytes);
        return true;
    }

    error_report(CS_INTERNAL_ERROR,
                 "program header containing desired address not found");
    return false;

elf_getphdrnum_fail:
    error_report(CS_INTERNAL_ERROR, "failed to get number of program headers");
    return CS_FAIL;

elf_getdata_rawchunk_fail:
    error_report(CS_INTERNAL_ERROR, "failed to read data from coredump file");
    return CS_FAIL;

size_too_big_fail:
    error_report(CS_INTERNAL_ERROR,
                 "requested mem chunk exceeds bounds of segment");
    return CS_FAIL;

}
