/******************************************************************************
 *
 *  File        : elf.h
 *  Description : ELF file format
 *
 *****************************************************************************/

#ifndef __FF_ELF_H__
#define __FF_ELF_H__

// Global typedefs
typedef  Uint32   elf32_addr;
typedef  Uint16   elf32_half;
typedef  Uint32   elf32_off;
typedef  Sint32   elf32_sword;
typedef  Uint32   elf32_word;


#define   ET_NONE    0      // No type
#define   ET_REL     1      // Relocatable file
#define   ET_EXEC    2      // Executable file
#define   ET_DYN     3      // Shared object
#define   ET_CORE    4      // Core file
#define   ET_LOPROC  0xFF00 // Processor specific
#define   ET_HIPROC  0xFFFF // Processor specific

#define   EM_NONE    0    // No machine
#define   EM_M32     1    // AT&T WE 32100
#define   EM_SPARC   2    // Sparc
#define   EM_386     3    // Intel 80386
#define   EM_68K     4    // Motorola 68000
#define   EM_88K     5    // Motorola 88000
#define   EM_860     7    // Intel 80860
#define   EM_MIPS    8    // MIPS RS3000

#define   EV_NONE    0    // Invalid version
#define   EV_CURRENT 1    // Current version

#define   EI_MAG0        0
#define   EI_MAG1        1
#define   EI_MAG2        2
#define   EI_MAG3        3
#define   EI_CLASS       4
#define   EI_DATA        5
#define   EI_VERSION     6
#define   EI_PAD         7
#define   EI_NIDENT     16

#define   ELFCLASSNONE    0
#define   ELFCLASS32      1
#define   ELFCLASS64      2

#define   ELFDATANONE     0
#define   ELFDATA2LSB     1
#define   ELFDATA2MSB     2

#define   SHN_UNDEF       0
#define   SHN_LORESERVE   0xFF00
#define   SHN_LOPROC      0xFF00
#define   SHN_HIPROC      0xFF1F
#define   SHN_ABS         0xFFF1
#define   SHN_COMMON      0xFFF2
#define   SHN_HIRESERVE   0xFFFF

#define   SHT_NULL        0
#define   SHT_PROGBITS    1
#define   SHT_SYMTAB      2
#define   SHT_STRTAB      3
#define   SHT_RELA        4
#define   SHT_HASH        5
#define   SHT_DYNAMIC     6
#define   SHT_NOTE        7
#define   SHT_NOBITS      8
#define   SHT_REL         9
#define   SHT_SHLIB      10
#define   SHT_DYNSYM     11
#define   SHT_LOPROC     0x70000000
#define   SHT_HIPROC     0x7FFFFFFF
#define   SHT_LOUSER     0x80000000
#define   SHT_HIUSER     0xFFFFFFFF


#define   ELF32_ST_BIND(i)   ((i)>>4)
#define   ELF32_ST_TYPE(i)   ((i)&0xF)
#define   ELF32_ST_INFO(b,t) ((b)<<4)+((t)&0xF))

#define   STB_LOCAL   0
#define   STB_GLOBAL  1
#define   STB_WEAK    2
#define   STB_LOPROC  13
#define   STB_HIPROC  15

#define   STT_NOTYPE    0
#define   STT_OBJECT    1
#define   STT_FUNC      2
#define   STT_SECTION   3
#define   STT_FILE      4
#define   STT_LOPROC    13
#define   STT_HIPROC    15

#define   ELF32_R_SYM(i)    ((i)>>8)
#define   ELF32_R_TYPE(i)   ((unsigned char)(i))
#define   ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

#define   R_386_NONE      0
#define   R_386_32        1
#define   R_386_PC32      2
#define   R_386_GOT32     3
#define   R_386_PLT32     4
#define   R_386_COPY      5
#define   R_386_GLOB_DAT  6
#define   R_386_JMP_SLOT  7
#define   R_386_RELATIVE  8
#define   R_386_GOTOFF    9
#define   R_386_GOTPC     10


#define   PT_NULL         0
#define   PT_LOAD         1
#define   PT_DYNAMIC      2
#define   PT_INTERP       3
#define   PT_NOTE         4
#define   PT_SHLIB        5
#define   PT_PHDR         6
#define   PT_LOPROC       0x70000000
#define   PT_HIPROC       0x7FFFFFFF

// Elf header structure
#pragma pack(1)
typedef struct {
  unsigned char     e_indent[EI_NIDENT];
  elf32_half        e_type;
  elf32_half        e_machine;
  elf32_word        e_version;
  elf32_addr        e_entry;
  elf32_off         e_phoff;
  elf32_off         e_shoff;
  elf32_word        e_flags;
  elf32_half        e_ehsize;
  elf32_half        e_phentsize;
  elf32_half        e_phnum;
  elf32_half        e_shentsize;
  elf32_half        e_shnum;
  elf32_half        e_shstrndx;
} elf32_ehdr;

#pragma pack(1)
typedef struct {
  elf32_word        sh_type;
  elf32_word        sh_flags;
  elf32_word        sh_addr;
  elf32_addr        sh_offset;
  elf32_off         sh_size;
  elf32_word        sh_link;
  elf32_word        sh_info;
  elf32_word        sh_addralign;
  elf32_word        sh_entsize;
} elf32_shdr;

#pragma pack(1)
typedef struct {
  elf32_word        st_name;
  elf32_addr        st_value;
  elf32_word        st_size;
  unsigned char     st_info;
  unsigned char     st_other;
  elf32_half        st_shndx;
} elf32_sym;

#pragma pack(1)
typedef struct {
  elf32_addr        r_offset;
  elf32_word        r_info;
} elf32_rel;

#pragma pack(1)
typedef struct {
  elf32_addr        r_offset;
  elf32_word        r_info;
  elf32_sword       r_addend;
} elf32_rela;

#pragma pack(1)
typedef struct {
  elf32_word        p_type;
  elf32_off         p_offset;
  elf32_addr        p_vaddr;
  elf32_addr        p_paddr;
  elf32_word        p_filesz;
  elf32_word        p_memsz;
  elf32_word        p_flags;
  elf32_word        p_align;
} elf32_phdr;


  int load_binary_elf (const char *path);

#endif //__FF_ELF_H__
