// include/elf.h
#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#define ELF_MAGIC 0x464C457F

typedef struct {
    uint32_t e_magic;      /* 0x7F 'E' 'L' 'F' */
    uint8_t  e_class;
    uint8_t  e_data;
    uint8_t  e_version;
    uint8_t  e_osabi;
    uint8_t  e_abiversion;
    uint8_t  pad[7];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version2;
    uint32_t e_entry;      /* entry point VA */
    uint32_t e_phoff;      /* program header table offset */
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;  /* size of one program header */
    uint16_t e_phnum;      /* number of program headers */
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf_header_t;

typedef struct {
    uint32_t p_type;   /* PT_LOAD = 1 */
    uint32_t p_offset; /* offset in file */
    uint32_t p_vaddr;  /* virtual address */
    uint32_t p_paddr;
    uint32_t p_filesz; /* bytes in file */
    uint32_t p_memsz;  /* bytes in memory */
    uint32_t p_flags;
    uint32_t p_align;
} elf_phdr_t;

#define PT_LOAD 1

#endif /* ELF_H */
