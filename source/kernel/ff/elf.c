/******************************************************************************
 *
 *  File        : elf.c
 *  Description : ELF format loader
 *
 *
 *****************************************************************************/
#include "kernel.h"
#include "vfs.h"
#include "ff/elf.h"
#include "kmem.h"

int elf_do_program_header (vfs_node_t *node, elf32_ehdr hdr);
int elf_do_section_header (vfs_node_t *node, elf32_ehdr hdr);
void elf_init_string_section (vfs_node_t *node, elf32_ehdr hdr);

char str_no_info[] = "no string info";
char *string_table_buffer = NULL;
Uint32 string_table_buffer_size = 0;


/**
 * Loads ELF binary. Returns entrypoint or 0 on error
 */
Uint32 load_binary_elf (const char *path) {
  elf32_ehdr hdr;

//  debug_vmm = 1;

  vfs_node_t node;
  if (! vfs_get_node_from_path (path, &node)) return 0;
  vfs_read (&node, 0, sizeof (elf32_ehdr), (char *)&hdr);

/*
  int i;
  kprintf ("=== ELF HEADER === \n");
  kprintf ("INDENT      : "); for (i=0; i!=EI_NIDENT; i++) kprintf ("%02X ", hdr.e_indent[i]); kprintf ("\n");
  kprintf ("TYPE        : %04X\n", hdr.e_type);
  kprintf ("MACHINE     : %04X\n", hdr.e_machine);
  kprintf ("VERSION     : %04X\n", hdr.e_version);
  kprintf ("ENTRY       : %04X\n", hdr.e_entry);
  kprintf ("PHOFF       : %04X\n", hdr.e_phoff);
  kprintf ("SHOFF       : %04X\n", hdr.e_shoff);
  kprintf ("FLAGS       : %04X\n", hdr.e_flags);
  kprintf ("e_ehsize    : %04X\n", hdr.e_ehsize);
  kprintf ("e_phentsize : %04X\n", hdr.e_phentsize);
  kprintf ("e_phnum     : %04X\n", hdr.e_phnum);
  kprintf ("e_shentsize : %04X\n", hdr.e_shentsize);
  kprintf ("e_shnum     : %04X\n", hdr.e_shnum);
  kprintf ("e_shstrndx  : %04X\n", hdr.e_shstrndx);
  kprintf ("\n");
*/

  // Must have ELF-signature
  if (hdr.e_indent[EI_MAG0] != 0x7F &&
      hdr.e_indent[EI_MAG1] != 'E' &&
      hdr.e_indent[EI_MAG2] != 'L' &&
      hdr.e_indent[EI_MAG3] != 'F') return 0;

  // Must be 32bit
  if (hdr.e_indent[EI_CLASS] != ELFCLASS32) return 0;

  // Must be little endian
  if (hdr.e_indent[EI_DATA] != ELFDATA2LSB) return 0;

  // Must be current elf spec
  if (hdr.e_indent[EI_VERSION] != EV_CURRENT) return 0;

  // Must be executable
  if (hdr.e_type != ET_EXEC) return 0;

  // Must be i386
  if (hdr.e_machine != EM_386) return 0;

  // Version 2 must also be current elf spec
  if (hdr.e_version != EV_CURRENT) return 0;

//  if (! elf_do_program_header (&node, hdr)) return 0;

  // Don't care about sections yet
 elf_init_string_section (&node, hdr);
 if (! elf_do_section_header (&node, hdr)) return 0;

  return hdr.e_entry;
}


/**
 *
 */
void elf_init_string_section (vfs_node_t *node, elf32_ehdr hdr) {
  if (! hdr.e_shstrndx) return;

  int i = hdr.e_shstrndx;

  Uint32 shtable_size = hdr.e_shnum * sizeof (elf32_shdr);
  elf32_shdr *shbuf = (elf32_shdr *)kmalloc (shtable_size);
  if (! shbuf) return;

  // Read section header table into buffer
  vfs_read (node, hdr.e_shoff, shtable_size, (char *)shbuf);
  elf32_shdr *sh_arr = (elf32_shdr *)shbuf;

  // Allocate string table buffer
  string_table_buffer = kmalloc (sh_arr[i].sh_size);
  string_table_buffer_size = sh_arr[i].sh_size;

  // Read string table into buffer buffer
  vfs_read (node, sh_arr[i].sh_offset, sh_arr[i].sh_size, (char *)string_table_buffer);

  // Free section header buffer
  kfree (shbuf);
}


/**
 *
 */
char *elf_get_section_string (Uint32 offset) {
  if (string_table_buffer == NULL) return str_no_info;

  if (offset > string_table_buffer_size) return str_no_info;
  return string_table_buffer+offset;
}


/**
 *
 */
int elf_do_section_header (vfs_node_t *node, elf32_ehdr hdr) {
  Uint32 phys_address;
  int i;

/*
  kprintf ("\n**** DOSECTIONHEADERS ***\n");
  kprintf ("=== ELF HEADER === \n");
  kprintf ("INDENT      : "); for (i=0; i!=EI_NIDENT; i++) kprintf ("%02X ", hdr.e_indent[i]); kprintf ("\n");
  kprintf ("TYPE        : %04X\n", hdr.e_type);
  kprintf ("MACHINE     : %04X\n", hdr.e_machine);
  kprintf ("VERSION     : %04X\n", hdr.e_version);
  kprintf ("ENTRY       : %04X\n", hdr.e_entry);
  kprintf ("PHOFF       : %04X\n", hdr.e_phoff);
  kprintf ("SHOFF       : %04X\n", hdr.e_shoff);
  kprintf ("FLAGS       : %04X\n", hdr.e_flags);
  kprintf ("e_ehsize    : %04X\n", hdr.e_ehsize);
  kprintf ("e_phentsize : %04X\n", hdr.e_phentsize);
  kprintf ("e_phnum     : %04X\n", hdr.e_phnum);
  kprintf ("e_shentsize : %04X\n", hdr.e_shentsize);
  kprintf ("e_shnum     : %04X\n", hdr.e_shnum);
  kprintf ("e_shstrndx  : %04X\n", hdr.e_shstrndx);
  kprintf ("\n");
*/

  // Allocate buffer for section headers
  Uint32 shtable_size = hdr.e_shnum * sizeof (elf32_shdr);
//  kprintf ("DSH: Reading %d bytes from %d\n", shtable_size, hdr.e_shoff);
  elf32_shdr *shbuf = (elf32_shdr *)kmalloc (shtable_size);
  if (! shbuf) return 0;

  // Read program haeder table into buffer
  vfs_read (node, hdr.e_shoff, shtable_size, (char *)shbuf);
  elf32_shdr *sh_ptr = shbuf;

  for (i=0; i!=hdr.e_shnum; i++, sh_ptr++) {
//    kprintf ("Buffer ptr: %08X\n", sh_ptr);

    // Didn't find an address or size
    if (! sh_ptr->sh_addr || ! sh_ptr->sh_size) continue;


/*
    kprintf ("sh_name      : %04X (%s)\n", sh_ptr->sh_name, elf_get_section_string (sh_ptr->sh_name));
    kprintf ("sh_type      : %04X\n", sh_ptr->sh_type);
    kprintf ("sh_flags     : %04X\n", sh_ptr->sh_flags);
    kprintf ("sh_addr      : %04X\n", sh_ptr->sh_addr);
    kprintf ("sh_offset    : %04X\n", sh_ptr->sh_offset);
    kprintf ("sh_size      : %04X\n", sh_ptr->sh_size);
    kprintf ("sh_link      : %04X\n", sh_ptr->sh_link);
    kprintf ("sh_info      : %04X\n", sh_ptr->sh_info);
    kprintf ("sh_addralign : %04X\n", sh_ptr->sh_addralign);
    kprintf ("sh_entsize   : %04X\n", sh_ptr->sh_entsize);
*/

    switch (sh_ptr->sh_type) {
      case SHT_PROGBITS :
        // @TODO: should be malloc()?
        kmalloc_pageboundary_physical (sh_ptr->sh_size, &phys_address);
//        kprintf (".text: %08X (%08X/%4x)\n", phys_address, sh_ptr->sh_addr, sh_ptr->sh_size);

        allocate_virtual_memory (phys_address, sh_ptr->sh_size, sh_ptr->sh_addr);
        vfs_read (node, sh_ptr->sh_offset, sh_ptr->sh_size, (char *)sh_ptr->sh_addr);

        break;
      case SHT_NOBITS :
        kmalloc_physical (sh_ptr->sh_size, &phys_address);
//        kprintf (".bss: %08X (%08X/%4x)\n", phys_address, sh_ptr->sh_addr, sh_ptr->sh_size);

        allocate_virtual_memory (phys_address, sh_ptr->sh_size, sh_ptr->sh_addr);
        memset ((char *)(sh_ptr->sh_offset), 0, sh_ptr->sh_size);
    }
  }

  kfree (shbuf);

  // Correctly loaded
  return 1;
}


/**
 *
 */
int elf_do_program_header (vfs_node_t *node, elf32_ehdr hdr) {
  int i;

//  kprintf ("\n**** DOPROGRAMHEADERS ***\n");

  // Allocate buffer for section headers
  Uint32 phtable_size = hdr.e_phnum * sizeof (elf32_phdr);
//  kprintf ("DSH: Reading %d bytes from %d\n", phtable_size, hdr.e_phoff);
  elf32_phdr *phbuf = (elf32_phdr *)kmalloc (phtable_size);
  if (! phbuf) return 0;

//  kprintf ("Buffer read: %08X\n", phbuf);

  // Read program haeder table into buffer
  vfs_read (node, hdr.e_phoff, phtable_size, (char *)phbuf);
  elf32_phdr *ph_ptr = phbuf;

//  kprintf ("Buffer ptr: %08X\n", ph_ptr);

  for (i=0; i!=hdr.e_phnum; i++) {
    kprintf ("Buffer ptr: %08X\n", ph_ptr);
    kprintf ("ph_type      : %04X\n", ph_ptr->p_type);
    kprintf ("ph_offset    : %04X\n", ph_ptr->p_offset);
    kprintf ("ph_vaddr     : %04X\n", ph_ptr->p_vaddr);
    kprintf ("ph_paddr     : %04X\n", ph_ptr->p_paddr);
    kprintf ("ph_filesz    : %04X\n", ph_ptr->p_filesz);
    kprintf ("ph_memsz     : %04X\n", ph_ptr->p_memsz);
    kprintf ("ph_flags     : %04X\n", ph_ptr->p_flags);
    kprintf ("ph_align     : %04X\n", ph_ptr->p_align);

    // No address, so nothing to add to there
    if (ph_ptr->p_type == PT_LOAD) {
      Uint32 phys_address;
      char *buffer = (char *)kmalloc_pageboundary_physical (ph_ptr->p_memsz, &phys_address);

      // We allocated a buffer, make sure this buffer has the correct virtual address
      kprintf ("Allocated memory onto %08X (%d bytes)\n", ph_ptr->p_vaddr, ph_ptr->p_memsz);
      allocate_virtual_memory (phys_address, ph_ptr->p_memsz, ph_ptr->p_vaddr);

      // Read ELF section into the virtual address (aka the buffer we just allocated)
      vfs_read (node, ph_ptr->p_offset, ph_ptr->p_filesz, (char *)ph_ptr->p_vaddr);

      // Rest of the memory is BSS. This has to be 0 filled.
      if (ph_ptr->p_memsz > ph_ptr->p_filesz) {
        memset ((char *)(buffer + ph_ptr->p_filesz), 0, ph_ptr->p_memsz - ph_ptr->p_filesz);
      }
    }

    // Next entry
    ph_ptr++;
  }

  kfree (phbuf);

  // Correctly loaded
  return 1;
}