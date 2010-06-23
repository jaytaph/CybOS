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

/*
int elf_add_section(unsigned int flags, unsigned long addr, unsigned long size, unsigned long offset) {
  printf ("elf_add_section (f: %04X a: %08X s: %08X o: %08X)\n", flags, addr, size, offset);
  return 0;
}

int main (void) {
  char buf[512];
  int i;

  // Open file
  FILE *f = fopen ("init", "rb");
  if (!f) return 0;

  // Read 1st sector
  fseek (f, 0, SEEK_SET);
  fread (buf, 1, sizeof (elf32_ehdr), f);

  elf32_ehdr *hdr = (elf32_ehdr *)buf;

  printf ("=== ELF HEADER === \n");
  printf ("INDENT      : "); for (i=0; i!=EI_NIDENT; i++) printf ("%02X ", hdr->e_indent[i]); printf ("\n");
  printf ("TYPE        : %04X\n", hdr->e_type);
  printf ("MACHINE     : %04X\n", hdr->e_machine);
  printf ("VERSION     : %04X\n", hdr->e_version);
  printf ("ENTRY       : %04X\n", hdr->e_entry);
  printf ("PHOFF       : %04X\n", hdr->e_phoff);
  printf ("SHOFF       : %04X\n", hdr->e_shoff);
  printf ("FLAGS       : %04X\n", hdr->e_flags);
  printf ("e_ehsize    : %04X\n", hdr->e_ehsize);
  printf ("e_phentsize : %04X\n", hdr->e_phentsize);
  printf ("e_phnum     : %04X\n", hdr->e_phnum);
  printf ("e_shentsize : %04X\n", hdr->e_shentsize);
  printf ("e_shnum     : %04X\n", hdr->e_shnum);
  printf ("e_shstrndx  : %04X\n", hdr->e_shstrndx);
  printf ("\n");

  // Must have ELF-signature
  if (hdr->e_indent[EI_MAG0] != 0x7F &&
      hdr->e_indent[EI_MAG1] != 'E' &&
      hdr->e_indent[EI_MAG2] != 'L' &&
      hdr->e_indent[EI_MAG3] != 'F') return 0;
  printf ("Signature found\n");

  // Must be 32bit
  if (hdr->e_indent[EI_CLASS] != ELFCLASS32) return 0;

  // Must be little endian
  if (hdr->e_indent[EI_DATA] != ELFDATA2LSB) return 0;

  // Must be current elf spec
  if (hdr->e_indent[EI_VERSION] != EV_CURRENT) return 0;

  // Must be executable
  if (hdr->e_type != ET_EXEC) return 0;

  // Must be i386
  if (hdr->e_machine != EM_386) return 0;

  // Version 2 must also be current elf spec
  if (hdr->e_version != EV_CURRENT) return 0;

  printf ("Looks good!\n\n");


  printf ("Entry point on %08X\n", hdr->e_entry);
  printf ("Program headers: %d on a size of %d\n", hdr->e_phnum, hdr->e_phentsize);
  printf ("Section headers: %d on a size of %d\n", hdr->e_shnum, hdr->e_shentsize);
  printf ("\n\n");

  do_program_headers (f, hdr);
  do_section_headers (f, hdr);

  fclose (f);
  printf ("\n");
  return 0;
}


int do_program_headers (FILE *f, elf32_ehdr *hdr) {
  int i,j;

  // Goto start of program headers and read into buffer
  fseek (f, hdr->e_phoff, SEEK_SET);
  elf32_phdr *phbuf = malloc (hdr->e_phnum * sizeof (elf32_phdr));
  fread (phbuf, hdr->e_phnum, sizeof (elf32_phdr), f);
  elf32_phdr *ph_ptr = phbuf;

  for (i=0; i!=hdr->e_phnum; i++) {
    printf ("Reading program header %d\n", i);
    printf ("p_type     : %04X\n", ph_ptr->p_type);
    printf ("p_offset   : %04X\n", ph_ptr->p_offset);
    printf ("p_vaddr    : %04X\n", ph_ptr->p_vaddr);
    printf ("p_paddr    : %04X\n", ph_ptr->p_paddr);
    printf ("p_filesz   : %04X\n", ph_ptr->p_filesz);
    printf ("p_memsz    : %04X\n", ph_ptr->p_memsz);
    printf ("p_flags    : %04X\n", ph_ptr->p_flags);
    printf ("p_align    : %04X\n", ph_ptr->p_align);

    switch (ph_ptr->p_type) {
      default         :
      case PT_NULL    :
      case PT_INTERP  :
      case PT_NOTE    :
      case PT_PHDR    :
      case PT_LOPROC  :
      case PT_HIPROC  :
                         // Do nothing
                         printf ("Skipping\n");
                         break;
      case PT_DYNAMIC :
      case PT_SHLIB   :
                         // Cannot handle these yet.
                         printf ("Cannot handle this...\n");
                         return 0;
                         break;
      case PT_LOAD    :
                         j = elf_add_section (1 | (ph_ptr->p_type & 0x7),
                                              ph_ptr->p_vaddr,
                                              ph_ptr->p_filesz,
                                              ph_ptr->p_offset);
                         if (j != 0) return 0;

                         // The memory size is larger than the filesize. Fill everything
                         // up with zero's (ie: BSS segment)
                         if (ph_ptr->p_memsz > ph_ptr->p_filesz) {
                           j = elf_add_section (0 | (ph_ptr->p_type & 0x7),
                                                ph_ptr->p_vaddr + ph_ptr->p_filesz,
                                                ph_ptr->p_memsz - ph_ptr->p_filesz,
                                                ph_ptr->p_offset);
                         }
                         break;
    }
    ph_ptr++;
  }

  free (phbuf);
  printf ("\n\n");
}


int do_section_headers (FILE *f, elf32_ehdr *hdr) {
  int i,j;

  // Goto start of program headers and read into buffer
  fseek (f, hdr->e_shoff, SEEK_SET);
  elf32_shdr *shbuf = malloc (hdr->e_shnum * sizeof (elf32_shdr));
  fread (shbuf, hdr->e_shnum, sizeof (elf32_shdr), f);
  elf32_shdr *sh_ptr = shbuf;

  for (i=0; i!=hdr->e_shnum; i++) {
    printf ("Reading section header %d\n", i);
    printf ("sh_type      : %04X\n", sh_ptr->sh_type);
    printf ("sh_flags     : %04X\n", sh_ptr->sh_flags);
    printf ("sh_addr      : %04X\n", sh_ptr->sh_addr);
    printf ("sh_offset    : %04X\n", sh_ptr->sh_offset);
    printf ("sh_size      : %04X\n", sh_ptr->sh_size);
    printf ("sh_link      : %04X\n", sh_ptr->sh_link);
    printf ("sh_info      : %04X\n", sh_ptr->sh_info);
    printf ("sh_addralign : %04X\n", sh_ptr->sh_addralign);
    printf ("sh_entsize   : %04X\n", sh_ptr->sh_entsize);

    sh_ptr++;
  }

  free (shbuf);
  printf ("\n\n");
}
*/


int do_program_header (vfs_node_t *node, elf32_ehdr hdr);
int do_section_header (vfs_node_t *node, elf32_ehdr hdr);

int elf_add_section(unsigned int flags, unsigned long addr, unsigned long size, unsigned long offset) {
  kprintf ("elf_add_section (f: %04X a: %08X s: %08X o: %08X)\n", flags, addr, size, offset);
  return 0;
}

/**
 *
 */
int load_binary_elf (const char *path) {
  elf32_ehdr hdr;
  int i;

  kprintf ("load ELF()\n");

  kprintf ("vfs node\n");
  vfs_node_t *node = vfs_get_node_from_path (path);
  if (! node) return 0;

  kprintf ("read header\n");
  vfs_read (node, 0, sizeof (elf32_ehdr), (char *)&hdr);

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


  if (! do_program_header (node, hdr)) return 0;
  if (! do_section_header (node, hdr)) return 0;

  return 1;
}

/**
 *
 */
int do_program_header (vfs_node_t *node, elf32_ehdr hdr) {
  int i,j;

  // Allocate buffer for program headers
  Uint32 phtable_size = hdr.e_phnum * sizeof (elf32_phdr);
  kprintf ("Reading %d bytes\n", phtable_size);
  elf32_phdr *buf = (elf32_phdr *)kmalloc (phtable_size);
  if (! buf) return 0;

  // Read program haeder table into buffer
  vfs_read (node, hdr.e_phoff, phtable_size, (char *)buf);
  elf32_phdr *ph_ptr = buf;

  for (i=0; i!=hdr.e_phnum; i++) {
    kprintf ("PTR: %08X\n", ph_ptr);
    kprintf ("Reading program header %d\n", i);
    kprintf ("p_type     : %04X\n", ph_ptr->p_type);
    kprintf ("p_offset   : %04X\n", ph_ptr->p_offset);
    kprintf ("p_vaddr    : %04X\n", ph_ptr->p_vaddr);
    kprintf ("p_paddr    : %04X\n", ph_ptr->p_paddr);
    kprintf ("p_filesz   : %04X\n", ph_ptr->p_filesz);
    kprintf ("p_memsz    : %04X\n", ph_ptr->p_memsz);
    kprintf ("p_flags    : %04X\n", ph_ptr->p_flags);
    kprintf ("p_align    : %04X\n", ph_ptr->p_align);

    switch (ph_ptr->p_type) {
      default         :
      case PT_NULL    :
      case PT_INTERP  :
      case PT_NOTE    :
      case PT_PHDR    :
      case PT_LOPROC  :
      case PT_HIPROC  :
                         // Do nothing
                         kprintf ("Skipping type %d\n", ph_ptr->p_type);
                         break;
      case PT_DYNAMIC :
      case PT_SHLIB   :
                         // Cannot handle these yet.
                         kprintf ("Cannot handle program header type %d...\n", ph_ptr->p_type);
                         kfree ((Uint32)buf);
                         return 0;
                         break;
      case PT_LOAD    :
                         j = elf_add_section (1 | (ph_ptr->p_type & 0x7),
                                              ph_ptr->p_vaddr,
                                              ph_ptr->p_filesz,
                                              ph_ptr->p_offset);
                         if (j != 0) {
                           kfree ((Uint32)buf);
                           return 0;
                         }

                         /* The memory size is larger than the filesize. Fill everything
                          * up with zero's (ie: BSS segment) */
                         if (ph_ptr->p_memsz > ph_ptr->p_filesz) {
                           j = elf_add_section (0 | (ph_ptr->p_type & 0x7),
                                                ph_ptr->p_vaddr + ph_ptr->p_filesz,
                                                ph_ptr->p_memsz - ph_ptr->p_filesz,
                                                ph_ptr->p_offset);
                         }
                         break;
    }
    ph_ptr++;
  }


  // Correctly loaded
  return 1;
}



/**
 *
 */
int do_section_header (vfs_node_t *node, elf32_ehdr hdr) {
  int i;

  kprintf ("\n**** DOSECTIONHEADERS ***\n");

  // Allocate buffer for section headers
  Uint32 shtable_size = hdr.e_shnum * sizeof (elf32_shdr);
  kprintf ("Reading %d bytes\n", shtable_size);
  elf32_shdr *buf = (elf32_shdr *)kmalloc (shtable_size);
  if (! buf) return 0;

  // Read program haeder table into buffer
  vfs_read (node, hdr.e_shoff, shtable_size, (char *)buf);
  elf32_shdr *sh_ptr = buf;

  for (i=0; i!=hdr.e_shnum; i++) {
    kprintf ("PTR: %08X\n", sh_ptr);
    kprintf ("sh_name       : %04X\n", sh_ptr->sh_name);
    kprintf ("sh_type      : %04X\n", sh_ptr->sh_type);
    kprintf ("sh_flags     : %04X\n", sh_ptr->sh_flags);
    kprintf ("sh_addr      : %04X\n", sh_ptr->sh_addr);
    kprintf ("sh_offset    : %04X\n", sh_ptr->sh_offset);
    kprintf ("sh_size      : %04X\n", sh_ptr->sh_size);
    kprintf ("sh_link      : %04X\n", sh_ptr->sh_link);
    kprintf ("sh_info      : %04X\n", sh_ptr->sh_info);
    kprintf ("sh_addralign : %04X\n", sh_ptr->sh_addralign);
    kprintf ("sh_entsize   : %04X\n", sh_ptr->sh_entsize);

    sh_ptr++;
  }


  // Correctly loaded
  return 1;
}