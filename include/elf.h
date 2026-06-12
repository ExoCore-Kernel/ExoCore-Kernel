#ifndef ELF_H
#define ELF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ELF_MAGIC 0x464C457F
#define ELF_CLASS_64 2
#define ELF_DATA_LSB 1
#define ELF_VERSION_CURRENT 1
#define ELF_TYPE_EXEC 2
#define ELF_MACHINE_X86_64 62
#define ELF_PT_LOAD 1
#define ELF_PF_X 1
#define ELF_PF_W 2
#define ELF_PF_R 4

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
    uint64_t e_entry;      /* entry point VA */
    uint64_t e_phoff;      /* program header table offset */
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;  /* size of one program header */
    uint16_t e_phnum;      /* number of program headers */
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header_t;

typedef struct {
    uint32_t p_type;   /* PT_LOAD = 1 */
    uint32_t p_flags;
    uint64_t p_offset; /* offset in file */
    uint64_t p_vaddr;  /* virtual address */
    uint64_t p_paddr;
    uint64_t p_filesz; /* bytes in file */
    uint64_t p_memsz;  /* bytes in memory */
    uint64_t p_align;
} __attribute__((packed)) elf_phdr_t;

typedef struct {
    uintptr_t requested_base;
    uintptr_t requested_end;
    void *load_base;
    size_t load_size;
    void *entry;
} elf_image_t;

int elf_validate(const void *file, size_t file_size);
int elf_load_process_image(int pid, const void *file, size_t file_size, elf_image_t *out);
void elf_free_process_image(int pid, elf_image_t *image);

#define PT_LOAD ELF_PT_LOAD

#ifdef __cplusplus
}
#endif

#endif /* ELF_H */
