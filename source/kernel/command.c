/******************************************************************************
 *
 *  File        : command.c
 *  Description : Construct Command functions
 *
 *****************************************************************************/
#include "command.h"
#include "errors.h"
#include "ktype.h"
#include "conio.h"
#include "gdt.h"

// Static descriptor types
char descriptortypes[][50] = {
                           "Data - Read Only - Expand up (protected data)",
                           "Data - Read Write - Expand up (normal data)",
                           "Data - Read Only - Expand down (protected stack)",
                           "Data - Read Write - Expand down (normal stack)",
                           "Code - No Read - Descriptor DPL",
                           "Code - No Read - Conforming DPL",
                           "Code - Read - Descriptor DPL",
                           "Code - Read - Conforming DPL"
                         };

/***************************************************************
 *  Prints the global descriptor table information on the console.
 *
 * In  : con_idx = index number of the console.
 */
int cmd_print_gdt (int con_idx)
{
  /*
  TGDTR tmp_gdtr;
  TGDT *descr;
  int i, gdt_base, gdt_limit;
  int gdt_g, gdt_d, gdt_p, gdt_s;
  int gdt_a, gdt_type, gdt_dpl;

  // Get global descriptor table record (U16 limit, U32 base)
  gdt_store_table (&tmp_gdtr);

  // Print basic info
  con_printf (con_idx, "GDT Information\n");
  con_printf (con_idx, "GDT Base  : %8x\n",  tmp_gdtr.base);
  con_printf (con_idx, "GDT Limit : %8x\n", tmp_gdtr.limit);

  con_printf (con_idx, "Skipping NULL descriptor\n");
  tmp_gdtr.base+=8;

  // Show all selectors in the GDT
  for (i=1; i!=tmp_gdtr.limit/8; i++)
  {
    // Get the decriptor data from the global data, since gdtr points to a
    // address that's originated from linear memory. 0x200 means 0x200, not
    // 0x20200 (address inside the kernel selectors). Therefore, we have to
    // copy the data from the global4gw descriptor.
    movedata (SEL(GLOBAL4GW_DATA_DESCR,TABLE_GDT+RING0),
              (unsigned int)tmp_gdtr.base,
              SEL(KERNEL_DATA_DESCR,TABLE_GDT+RING0),
              (unsigned int)descr,
              8);

    // Calc descriptor info
    gdt_base=(int)descr->BaseLo+(descr->BaseMid << 16)+(descr->BaseHi<<24);
    gdt_limit=(int)descr->LimitLo+((descr->Limit_Flags2 & 0x0F) << 16);

    gdt_g=((descr->Limit_Flags2 & 0x80) >> 7);
    gdt_d=(int)((descr->Limit_Flags2 & 0x40) >> 6);
    gdt_p=(int)((descr->Flags1 & 0x80) >> 7);
    gdt_s=(int)((descr->Flags1 & 0x10) >> 4);
    gdt_a=(int)(descr->Flags1 & 0x01);
    gdt_type=(int)((descr->Flags1 & 0x0D) >> 1);
    gdt_dpl=(int)((descr->Flags1 & 0x60) >> 5);

    // And print it
    con_printf (con_idx, "Idx: %d | Base %x | Limit %x | G: %d | D: %d | P: %d | A: %d | S: %d \n", i, gdt_base, gdt_limit, gdt_g, gdt_d, gdt_p, gdt_a, gdt_s);
    con_printf (con_idx, "Type: (%d) %s\n", gdt_type, descriptortypes[gdt_type]);

    // Next descriptor
    tmp_gdtr.base+=8;
  }

  error_seterror (ERR_OK);
*/
  return ERR_OK;
}

