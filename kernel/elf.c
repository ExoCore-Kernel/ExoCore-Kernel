#include "elf.h"
#include "proc.h"
#include "memutils.h"

#define ELF_MAX_LOAD_SIZE (16U * 1024U * 1024U)

static int add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out) {
    *out = a + b;
    return *out < a;
}

static int range_inside(uint64_t off, uint64_t len, size_t total) {
    uint64_t end;
    if (add_overflow_u64(off, len, &end))
        return 0;
    return off <= (uint64_t)total && end <= (uint64_t)total;
}

static int valid_flags(uint32_t flags) {
    return (flags & ~(ELF_PF_R | ELF_PF_W | ELF_PF_X)) == 0;
}

static int inspect_load_range(const elf_header_t *eh, const elf_phdr_t *ph, size_t file_size,
                              uint64_t *out_min, uint64_t *out_max, int *out_loads) {
    uint64_t min_vaddr = UINT64_MAX;
    uint64_t max_vaddr = 0;
    int loads = 0;

    for (uint16_t i = 0; i < eh->e_phnum; ++i) {
        if (ph[i].p_type != ELF_PT_LOAD)
            continue;
        uint64_t file_end;
        uint64_t mem_end;
        if (!valid_flags(ph[i].p_flags) || ph[i].p_memsz == 0 || ph[i].p_filesz > ph[i].p_memsz)
            return -1;
        if (add_overflow_u64(ph[i].p_offset, ph[i].p_filesz, &file_end) || file_end > (uint64_t)file_size)
            return -1;
        if (add_overflow_u64(ph[i].p_vaddr, ph[i].p_memsz, &mem_end))
            return -1;
        if (ph[i].p_align && (ph[i].p_align & (ph[i].p_align - 1)) != 0)
            return -1;
        if (ph[i].p_align > 1 && ((ph[i].p_vaddr - ph[i].p_offset) & (ph[i].p_align - 1)) != 0)
            return -1;
        if (ph[i].p_vaddr < min_vaddr)
            min_vaddr = ph[i].p_vaddr;
        if (mem_end > max_vaddr)
            max_vaddr = mem_end;
        ++loads;
    }

    if (loads == 0 || eh->e_entry < min_vaddr || eh->e_entry >= max_vaddr)
        return -1;
    if (max_vaddr <= min_vaddr || max_vaddr - min_vaddr > ELF_MAX_LOAD_SIZE)
        return -1;
    *out_min = min_vaddr;
    *out_max = max_vaddr;
    *out_loads = loads;
    return 0;
}

int elf_validate(const void *file, size_t file_size) {
    if (!file || file_size < sizeof(elf_header_t))
        return -1;
    const elf_header_t *eh = (const elf_header_t*)file;
    if (eh->e_magic != ELF_MAGIC || eh->e_class != ELF_CLASS_64 || eh->e_data != ELF_DATA_LSB)
        return -1;
    if (eh->e_version != ELF_VERSION_CURRENT || eh->e_version2 != ELF_VERSION_CURRENT)
        return -1;
    if (eh->e_type != ELF_TYPE_EXEC || eh->e_machine != ELF_MACHINE_X86_64)
        return -1;
    if (eh->e_ehsize != sizeof(elf_header_t) || eh->e_phentsize != sizeof(elf_phdr_t) || eh->e_phnum == 0)
        return -1;
    if (!range_inside(eh->e_phoff, (uint64_t)eh->e_phentsize * eh->e_phnum, file_size))
        return -1;

    const elf_phdr_t *ph = (const elf_phdr_t*)((const unsigned char*)file + eh->e_phoff);
    uint64_t min_vaddr, max_vaddr;
    int loads;
    return inspect_load_range(eh, ph, file_size, &min_vaddr, &max_vaddr, &loads);
}

int elf_load_process_image(int pid, const void *file, size_t file_size, elf_image_t *out) {
    if (!out || elf_validate(file, file_size) != 0)
        return -1;
    const elf_header_t *eh = (const elf_header_t*)file;
    const elf_phdr_t *ph = (const elf_phdr_t*)((const unsigned char*)file + eh->e_phoff);
    uint64_t min_vaddr, max_vaddr;
    int loads;
    if (inspect_load_range(eh, ph, file_size, &min_vaddr, &max_vaddr, &loads) != 0)
        return -1;
    (void)loads;

    size_t load_size = (size_t)(max_vaddr - min_vaddr);
    unsigned char *base = proc_alloc(pid, load_size);
    if (!base)
        return -1;
    memset(base, 0, load_size);

    for (uint16_t i = 0; i < eh->e_phnum; ++i) {
        if (ph[i].p_type != ELF_PT_LOAD)
            continue;
        size_t dest_off = (size_t)(ph[i].p_vaddr - min_vaddr);
        if (dest_off > load_size || ph[i].p_memsz > load_size - dest_off) {
            proc_free(pid, base);
            return -1;
        }
        memcpy(base + dest_off, (const unsigned char*)file + ph[i].p_offset, (size_t)ph[i].p_filesz);
    }

    out->requested_base = (uintptr_t)min_vaddr;
    out->requested_end = (uintptr_t)max_vaddr;
    out->load_base = base;
    out->load_size = load_size;
    out->entry = base + (eh->e_entry - min_vaddr);
    return 0;
}

void elf_free_process_image(int pid, elf_image_t *image) {
    if (!image || !image->load_base)
        return;
    proc_free(pid, image->load_base);
    memset(image, 0, sizeof(*image));
}
