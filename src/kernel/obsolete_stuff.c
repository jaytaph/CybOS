// =======
void do_general_protection_fault (stackregs r, Uint32 errcode, Uint32 eip, Uint32 cs, Uint32 eflags) {
  int gpf_cause_index=0;
  Uint32 reg_ss, reg_esp;
  Uint32 reg_cs, reg_eip;
  Uint32 reg_ds, reg_es, reg_fs, reg_gs;
  Uint32 reg_cr0, reg_cr2, reg_cr3;
  Uint16 reg_tr;

  TGDTR tmp_gdtr;
  TIDTR tmp_idtr;

  errcode=errcode;

  // Load the vars with the data segments
  __asm__ __volatile__ (
    "mov %%ds, %0 \n\t"
    "mov %%es, %1 \n\t"
    "mov %%fs, %2 \n\t"
    "mov %%gs, %3 \n\t"
    "mov %%ss, %4 \n\t"
    : "=r" (reg_ds), "=r" (reg_es), "=r" (reg_fs), "=r" (reg_gs), "=r" (reg_ss)
  );

  // Load the vars with the control registers
  __asm__ __volatile__ (
    "mov %%cr0, %0 \n\t"
    "mov %%cr2, %1 \n\t"
    "mov %%cr3, %2 \n\t"
    : "=r" (reg_cr0), "=r" (reg_cr2), "=r" (reg_cr3)
  );

  // More vars
//  reg_esp=r.esp+16;
  reg_esp=r.esp;
  reg_cs=cs;
  reg_eip=eip;

  // Make a nice blueish screen (tm)
  con_setattr (1, 1*16+7);
  con_clrscr (1);

  // Show centered welcome message
  con_setattr (CON_GPF_IDX, 7*16+1);
  con_gotoxy (CON_GPF_IDX, 40-(strlen (BSOD)/2), 1);
  con_printf (CON_GPF_IDX, BSOD);
  con_setattr (CON_GPF_IDX, 1*16+7);

  // print basic registers
  con_gotoxy (CON_GPF_IDX,  1, 3); con_printf (CON_GPF_IDX, "EAX: %08X", r.eax);
  con_gotoxy (CON_GPF_IDX, 20, 3); con_printf (CON_GPF_IDX, "EBX: %08X", r.ebx);
  con_gotoxy (CON_GPF_IDX, 40, 3); con_printf (CON_GPF_IDX, "ECX: %08X", r.ecx);
  con_gotoxy (CON_GPF_IDX, 60, 3); con_printf (CON_GPF_IDX, "EDX: %08X", r.edx);

  con_gotoxy (CON_GPF_IDX,  1, 4); con_printf (CON_GPF_IDX, "ESI: %08X", r.esi);
  con_gotoxy (CON_GPF_IDX, 20, 4); con_printf (CON_GPF_IDX, "EDI: %08X", r.edi);
  con_gotoxy (CON_GPF_IDX, 40, 4); con_printf (CON_GPF_IDX, "EBP: %08X", r.ebp);
  con_gotoxy (CON_GPF_IDX, 60, 4); con_printf (CON_GPF_IDX, "FLG: %08X", eflags);

  con_gotoxy (CON_GPF_IDX,  1, 5); con_printf (CON_GPF_IDX, "CR0: %08X", reg_cr0);
  con_gotoxy (CON_GPF_IDX, 20, 5); con_printf (CON_GPF_IDX, "CR1: ________");
  con_gotoxy (CON_GPF_IDX, 40, 5); con_printf (CON_GPF_IDX, "CR2: %08X", reg_cr2);
  con_gotoxy (CON_GPF_IDX, 60, 5); con_printf (CON_GPF_IDX, "CR3: %08X", reg_cr3);

  con_gotoxy (CON_GPF_IDX,  1, 6); con_printf (CON_GPF_IDX, "EIP: %08X", eip);
  con_gotoxy (CON_GPF_IDX, 20, 6); con_printf (CON_GPF_IDX, "ESP: %08X", reg_esp);

//  ////////////////// TRY TO FIND OUT WHY WE CRASHED ////////////////////////
//  // Software Interrupt 13?
//  movedata (reg_cs, reg_eip, SEL(GLOBAL4GW_DATA_DESCR,TABLE_GDT+RING0), (unsigned int)&tmp, 2);
//  if (tmp[0]==0xCD || tmp[1]==0x0D) gpf_cause_index=1;
//
//  // Loaded NULL var (so it's probably used)
//  if (reg_ds==0 || reg_es==0 || reg_fs==0 || reg_gs==0)  gpf_cause_index=2;
//
//  ////////////////// TRY TO FIND OUT WHY WE CRASHED ////////////////////////

  // Print propable cause of GPF
  con_gotoxy (CON_GPF_IDX, 40, 6);
  con_printf (CON_GPF_IDX, "%s", gpf_cause[gpf_cause_index]);

  // Show selectors
  con_gotoxy (CON_GPF_IDX, 1, 9);
  con_printf (CON_GPF_IDX, "SEG sltr (index|ti|rpl)     base    limit G D");
  con_gotoxy (CON_GPF_IDX, 1, 10);
  con_printf (CON_GPF_IDX, " CS:%04X ( %04X| %d|  %d) %08X %08X - -",
              reg_cs, (reg_cs >> 3), (reg_cs & 1), ((reg_cs >> 1) & 3),
              gdt_getbase (reg_cs), gdt_getlimit (reg_cs));
  con_gotoxy (CON_GPF_IDX, 1, 11);
  con_printf (CON_GPF_IDX, " DS:%04X ( %04X| %d|  %d) %08X %08X - -",
              reg_ds, (reg_ds >> 3), (reg_ds & 1), ((reg_ds >> 1) & 3),
              gdt_getbase (reg_ds), gdt_getlimit (reg_ds));
  con_gotoxy (CON_GPF_IDX, 1, 12);
  con_printf (CON_GPF_IDX, " ES:%04X ( %04X| %d|  %d) %08X %08X - -",
              reg_es, (reg_es >> 3), (reg_es & 1), ((reg_es >> 1) & 3),
              gdt_getbase (reg_es), gdt_getlimit (reg_es));
  con_gotoxy (CON_GPF_IDX, 1, 13);
  con_printf (CON_GPF_IDX, " FS:%04X ( %04X| %d|  %d) %08X %08X - -",
              reg_fs, (reg_fs >> 3), (reg_fs & 1), ((reg_fs >> 1) & 3),
              gdt_getbase (reg_fs), gdt_getlimit (reg_fs));
  con_gotoxy (CON_GPF_IDX, 1, 14);
  con_printf (CON_GPF_IDX, " GS:%04X ( %04X| %d|  %d) %08X %08X - -",
              reg_gs, (reg_gs >> 3), (reg_gs & 1), ((reg_gs >> 1) & 3),
              gdt_getbase (reg_gs), gdt_getlimit (reg_gs));
  con_gotoxy (CON_GPF_IDX, 1, 15);
  con_printf (CON_GPF_IDX, " SS:%04X ( %04X| %d|  %d) %08X %08X - -",
              reg_ss, (reg_ss >> 3), (reg_ss & 1), ((reg_ss >> 1) & 3),
              gdt_getbase (reg_ss), gdt_getlimit (reg_ss));

/*
  // Table registers
  gdt_store_table (&tmp_gdtr);
  idt_store_table ((unsigned char *)&tmp_idtr);
  __asm__ __volatile__ ("str (%%eax)" : : "a" (&reg_tr));
  con_gotoxy (CON_GPF_IDX, 50, 10); con_printf (CON_GPF_IDX, "GDT: %08X %04X", tmp_gdtr.base, tmp_gdtr.limit);
  con_gotoxy (CON_GPF_IDX, 50, 11); con_printf (CON_GPF_IDX, "IDT: %08X %04X", tmp_idtr.base, tmp_idtr.limit);
  con_gotoxy (CON_GPF_IDX, 50, 12); con_printf (CON_GPF_IDX, "LDT: ________ ____");
  con_gotoxy (CON_GPF_IDX, 50, 13); con_printf (CON_GPF_IDX, "TR : %04X", reg_tr);

  // Stack dump
  con_gotoxy (CON_GPF_IDX, 1, 17); con_printf (CON_GPF_IDX, "Stack dump: (SS:ESP)");
  con_gotoxy (CON_GPF_IDX, 1, 18); dumpmem (reg_ss, (Uint32 *)&reg_esp, 0x8FFFF, 1);
  con_gotoxy (CON_GPF_IDX, 1, 19); dumpmem (reg_ss, (Uint32 *)&reg_esp, 0x8FFFF, 1);
  con_gotoxy (CON_GPF_IDX, 1, 20); dumpmem (reg_ss, (Uint32 *)&reg_esp, 0x8FFFF, 1);
  con_gotoxy (CON_GPF_IDX, 1, 21); dumpmem (reg_ss, (Uint32 *)&reg_esp, 0x8FFFF, 1);
  con_gotoxy (CON_GPF_IDX, 1, 22); dumpmem (reg_ss, (Uint32 *)&reg_esp, 0x8FFFF, 1);
  con_gotoxy (CON_GPF_IDX, 1, 23); dumpmem (reg_ss, (Uint32 *)&reg_esp, 0x8FFFF, 1);

  // Code dump
  con_gotoxy (CON_GPF_IDX, 40, 17); con_printf (1, "Code dump: (CS:EIP)");
  con_gotoxy (CON_GPF_IDX, 40, 18); dumpmem (reg_cs, (Uint32 *)&eip, 0, -1);
  con_gotoxy (CON_GPF_IDX, 40, 19); dumpmem (reg_cs, (Uint32 *)&eip, 0, -1);
  con_gotoxy (CON_GPF_IDX, 40, 20); dumpmem (reg_cs, (Uint32 *)&eip, 0, -1);
  con_gotoxy (CON_GPF_IDX, 40, 21); dumpmem (reg_cs, (Uint32 *)&eip, 0, -1);
  con_gotoxy (CON_GPF_IDX, 40, 22); dumpmem (reg_cs, (Uint32 *)&eip, 0, -1);
  con_gotoxy (CON_GPF_IDX, 40, 23); dumpmem (reg_cs, (Uint32 *)&eip, 0, -1);

  // Now show it on our screen
  switch_console (CON_GPF_IDX, 1);
*/

  // Deadlock
deadlocked:
  goto deadlocked;
}





/********************************************************
 * Default General Protection Fault Handler
 */
unsigned char BSOD[] = " Blue Screen O' Death ";

unsigned char *gpf_cause[] =
  {
    "Undetectable",
    "Software interrupt INT 13 (CD0D)"
    "Segment register loaded with NULL",
  };

typedef struct {
  unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
} stackregs;

void dumpmem (unsigned int sel, Uint32 *off, unsigned long limit, signed int delta) {
  int i,j;
  unsigned char val[4];
  unsigned char ok[4];
  unsigned int baseaddr,oldbaseaddr;

  // Get linear address
  baseaddr=gdt_getbase (sel);
  baseaddr+=*off;
  oldbaseaddr=baseaddr;

  for (j=0; j!=4; j++)
  {
    ok[0]=0; ok[1]=0; ok[2]=0; ok[3]=0;

    con_printf (1, " ");

    for (i=0; i!=4; i++) {
      if (baseaddr==limit) {
        // Cannot read memory when it's 0
        ok[i]=0;
      } else {
        movedata (SEL(GLOBAL4GW_DATA_DESCR,TABLE_GDT+RING0), (unsigned int)baseaddr,
                  SEL(GLOBAL4GW_DATA_DESCR,TABLE_GDT+RING0), (Uint32)&val[i],
                  1);
        ok[i]=1;
        baseaddr+=delta;
      }
    }
    ok[3]?con_printf (1, "%02X", val[3]):con_printf (CON_GPF_IDX, "--");
    ok[2]?con_printf (1, "%02X", val[2]):con_printf (CON_GPF_IDX, "--");
    ok[1]?con_printf (1, "%02X", val[1]):con_printf (CON_GPF_IDX, "--");
    ok[0]?con_printf (1, "%02X", val[0]):con_printf (CON_GPF_IDX, "--");
  }

  // offset must be set to the correct address.
  *off-=(oldbaseaddr-baseaddr)*delta;
}







/****************************************************************************
 * Initializes the kernel's global descriptor table. We currently add the
 * "useless" descriptors to the GDT, but we don't fill the descriptor-vars.
 * If evertything works, we remove these descriptors, the other descriptors
 * will lower in value and stuff should work also..
 */
int gdt_init (void) {
/*
  // Allocate room for the kernel_gdt
  // _kernel_gdt=(unsigned char *)k_malloc (MAX_DESCRIPTORS*8);
  memset (_kernel_gdt, 0x0, MAX_DESCRIPTORS);

  // NULL descriptor, set everything to zero
  gdt_set (NULL_DESCR,           0x000000000000000ULL);
  gdt_set (GLOBAL4GW_CODE_DESCR, 0x000000000000000ULL);
  gdt_set (GLOBAL4GW_DATA_DESCR, 0x000000000000000ULL);
  gdt_set (KERNEL_CODE_DESCR,    0x000000000000000ULL);
  gdt_set (KERNEL_DATA_DESCR,    0x000000000000000ULL);
  gdt_set (TSS_TASK_DESCR,       0x000000000000000ULL);
  gdt_set (LDT_TASK_DESCR,       0x000000000000000ULL);

  // Create other descriptors
  VGA_DESCR            = gdt_add_descriptor (VGA_DESCR,                0xB8000, 0xFFFF,            GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_DATA+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);
  GLOBAL4GW_CODE_DESCR = gdt_add_descriptor (GLOBAL4GW_CODE_DESCR,     0x0, 0xFFFFF,           GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_CODE+GDT_ACCESSED, GDT_GRANULARITY+GDT_BITS32);
  GLOBAL4GW_DATA_DESCR = gdt_add_descriptor (GLOBAL4GW_DATA_DESCR,     0x0, 0xFFFFF,           GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_DATA+GDT_ACCESSED, GDT_GRANULARITY+GDT_BITS32);
  BOOT_CODE_DESCR      = gdt_add_descriptor (BOOT_CODE_DESCR,          0x7E00, 0x1FF,             GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_CODE+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);
  BOOT_DATA_DESCR      = gdt_add_descriptor (BOOT_DATA_DESCR,          0x7E00, 0x1FF,             GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_DATA+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);
  KRNL_LDR_CODE_DESCR  = gdt_add_descriptor (KRNL_LDR_CODE_DESCR,          0x8000, 0xFFF,             GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_CODE+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);
  KRNL_LDR_DATA_DESCR  = gdt_add_descriptor (KRNL_LDR_DATA_DESCR,         0x8000, 0xFFF,             GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_DATA+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);
  KERNEL_CODE_DESCR    = gdt_add_descriptor (KERNEL_CODE_DESCR, KERNEL_PHYS_BASE, KERNEL_PHYS_LIMIT, GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_CODE+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);
  KERNEL_DATA_DESCR    = gdt_add_descriptor (KERNEL_DATA_DESCR, KERNEL_PHYS_BASE, KERNEL_PHYS_LIMIT, GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_DATA+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);
  KERNEL_STACK_DESCR   = gdt_add_descriptor (KERNEL_STACK_DESCR, KERNEL_PHYS_BASE, KERNEL_PHYS_LIMIT, GDT_PRESENT+GDT_DPL_RING0+GDT_SEGMENT_CD+GDT_TYPE_RW_UP_DATA+GDT_ACCESSED, GDT_NO_GRANULARITY+GDT_BITS32);

  // damn... stack, code and data *MUST* be always the same as in the
  // boot-loader. If the stack is different, we cannot relocate our stack-
  // values that are currently on our stack. later ff uitzoeken dus..
  gdt_load_table (_kernel_gdt, MAX_DESCRIPTORS);

  __asm__ __volatile__ ("movw %%ax, %%es" : : "a" (SEL(KERNEL_DATA_DESCR,TABLE_GDT+RING0)));
  __asm__ __volatile__ ("movw %%ax, %%fs" : : "a" (SEL(KERNEL_DATA_DESCR,TABLE_GDT+RING0)));
  __asm__ __volatile__ ("movw %%ax, %%gs" : : "a" (SEL(KERNEL_DATA_DESCR,TABLE_GDT+RING0)));

  __asm__ __volatile__ ("movw %%ax, %%ss" : : "a" (SEL(KERNEL_STACK_DESCR,TABLE_GDT+RING0)));
  __asm__ __volatile__ ("movw %%ax, %%ds" : : "a" (SEL(KERNEL_DATA_DESCR,TABLE_GDT+RING0)));
*/
  error_seterror (ERR_OK);
  return ERR_OK;
}

/****************************************************************************
 * Returns the first available descriptor inside the GDT
 *
 * In:  descriptor   = gdt descriptor struct to be filled when an empty one if found
 *      slot_start   = index on where it starts the search
 *      slot_end     = index on where to cancel the search
 *
 * Out: index of free descriptor or -1 on error
 *      descriptor points to a free TGDT struct
 */
int gdt_get_free_gdt_descriptor (int slot_start, int slot_end) {
  int slot = slot_start;
  TGDT *d;

  // Let the *descriptor points to the descriptor we have choosen in slot_start
  d = &_kernel_gdt[slot_start];

  // We use the present-bit to find out if a descriptor is present in this slot.
  // It should work since we don't remove
  while ( ( (d->Flags1 & GDT_PRESENT) == GDT_PRESENT)  && (slot<=slot_end) ) {
    d++;
    slot++;
  }

  // Slot found exceeds the limit we set to the search
  if (slot>slot_end) {
    error_seterror (ERR_GDT_TABLEFULL);
    *descriptor = 0x0;
    return ERR_ERROR;
  }

  // Alles kla!
  error_seterror (ERR_OK);

  *descriptor = d;
  return slot;
}


/****************************************************************************
 * Adds a descriptor to the GDT
 *
 * In :
 *        base     = base of the descriptor
 *        limit    = limit of the descriptor
 *        flags1   = 1st flagset of the descriptor (use defines in gdt.h)
 *        flags2   = 2nd flagset of the descriptor (use defines in gdt.h)
 *
 * Out: descriptor value (already multiplied by 8) or -1 on error.
 */
int gdt_add_descriptor (Uint32 base, Uint32 limit, Uint8 flags1, Uint8 flags2) {
  int free_slot;
  TGDT *descr;

  // Fetch a free slot
  free_slot = gdt_get_free_descriptor (&descr, FIRST_GDT_DESCRIPTOR, LAST_GDT_DESCRIPTOR);
  if (free_slot==ERR_ERROR) {
    error_seterror (ERR_GDT_TABLEFULL);
    return ERR_ERROR;
  }

  // Set items
  descr->BaseHi  = HI8 (HI16 (base));
  descr->BaseMid = LO8 (HI16 (base));
  descr->BaseLo  = LO16 (base);
  descr->LimitLo = LO16 (limit);
  descr->Limit_Flags2 = (LO8(HI16 (limit)) & 0x0F);
  descr->Limit_Flags2 += (LO8(flags2) & 0xF0);
  descr->Flags1  = LO8(flags1);

  gdt_set (free_slot, (unsigned long long *)descr);

  error_seterror (ERR_OK);
  return free_slot;
}



/****************************************************************************
 * Removes a descriptor from the GDT by filling it with blanks.
 *
 * In : descriptor index to clear
 *
 * Out: -1 on error
 */
int gdt_remove_descriptor (unsigned int descriptor_slot) {
  _kernel_gdt[descriptor_slot] = 0;

  error_seterror (ERR_OK);
  return ERR_OK;
}




/****************************************************************************
 * Loads the global descriptor table.
 *
 * In :   gdt_base = pointer to the kernel structure
 *        size     = size of the complete structure (in bytes)
 */
int gdt_load_table (void *gdt_base, Uint16 size) {
  TGDTR gdtr;

  gdtr.limit = (Uint16)(size*8*2)-1;
  gdtr.base  = (Uint32)gdt_base+KERNEL_PHYS_BASE;

  __asm__ __volatile__ ("lgdt (%0)" : : "p" (&gdtr));

  error_seterror (ERR_OK);
  return ERR_OK;
}

/****************************************************************************
 * Retrieves the global descriptor table.
 *
 * In  : pointer to store the GDTR
 * Out : -1 on errorn

 */
int gdt_store_table (TGDTR *gdtr) {
  __asm__ __volatile__ ("sgdt (%0)" : : "p" (gdtr));

  error_seterror (ERR_OK);
  return ERR_OK;
}

/****************************************************************************
 * Get the base address of a selector
 *
 * In:  selector = selector taken from segment register
 *
 * Out: Base or -1 on error
 */
unsigned int gdt_getbase (unsigned int selector) {
  TGDT *descr = 0;
  TGDTR tmp_gdtr;

  // Get gdtr structure
  gdt_store_table (&tmp_gdtr);

  // Get address of descriptor pointed in selector. Relocate in kernel mem.
  descr=(TGDT *)(tmp_gdtr.base+(selector & 0xF8)-KERNEL_PHYS_BASE);

  error_seterror (ERR_OK);
  return (unsigned int)descr->BaseLo+(descr->BaseMid << 16)+(descr->BaseHi<<24);
}

/****************************************************************************
 * Get the limit of a selector.
 *
 * In:  selector = selector taken from segment register
 *
 * Out: Limit or -1 on error
 */
unsigned int gdt_getlimit (unsigned int selector) {
  TGDT *descr;
  TGDTR tmp_gdtr;

  // Get gdtr structure
  gdt_store_table (&tmp_gdtr);

  // Get address of descriptor pointed in selector
  descr=(TGDT *)(tmp_gdtr.base+(selector & 0xF8)-KERNEL_PHYS_BASE);

  error_seterror (ERR_OK);
  return (unsigned int)descr->LimitLo+((descr->Limit_Flags2 & 0x0F) << 16);
}

/****************************************************************************
 * Get the flags of a selector.
 *
 * In:  selector = selector taken from segment register
 *
 * Out: Flags1(LO8)+Flags2(HI8) or -1 on error
 */
unsigned int gdt_getflags (unsigned int selector) {
  TGDT *descr;
  TGDTR tmp_gdtr;

  // Get gdtr structure
  gdt_store_table (&tmp_gdtr);

  // Get address of descriptor pointed in selector
  descr=(TGDT *)(tmp_gdtr.base+(selector & 0xF8)-KERNEL_PHYS_BASE);

  error_seterror (ERR_OK);
  return (unsigned int)(descr->Limit_Flags2 & 0xF0)+(descr->Flags1 & 0x0F);
}

