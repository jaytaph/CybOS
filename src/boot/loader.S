;***************************************************************
;* This file is loaded by the boot-loader and can be           *
;* maximum 32 kb in size. To be exactly, it loads up to 64     *
;* sectors. I don't want to mess with segment boundaries       *
;* in realmode, so we just stick to 32kb. If you can't load it *
;* in 32kb, forget about it.. :-)                              *
;***************************************************************


[bits 16]
[org 0x8000]

; Do not add code before this label
KRNL_LDR_START:
        ; We are loaded on 0800:0000
        jmp Start                ; Jump over the data

; --------------------------------
; Data used in the file
; --------------------------------

        ; Kernel Size must be at offset 10 as to be the loading number of
        ; sectors by boot.S
        times 10-($-$$) db 0x00
        KERNEL_SECTOR_SIZE db ((KRNL_LDR_END-KRNL_LDR_START)/512)+1

%include "globals.def"

        PAGEDIR           equ 0x9F000    ; Pointer to page directory
        PAGETABLE_1MB     equ 0x9E000    ; Pointer to pagetable of 1:1 mapping of 1st MB
        KERNEL_IDT_ADDR   equ 0x9C000    ; Pointer to idt addr (this should be enough room for IDT and GDT)

        NULL_SELECTOR            equ 0x0
        KERNEL_CODE_SELECTOR     equ 0x8
        KERNEL_DATA_SELECTOR     equ 0x10

        GDT_TABLE:
          dq 0x0000000000000000    ; null descriptor
          dq 0x00cf9a000000ffff    ; kernel code (complete memory)
          dq 0x00cf92000000ffff    ; kernel data (complete memory)
          dq 0x0000000000000000    ; TSS selector (filled later on)
          dq 0x0000000000000000    ; LDT selector (filled later on)

        gdt_size EQU $-GDT_TABLE    ; Size of the GDT table

        LGDT_STRUC:
          dw  gdt_size    ; Length of the GDT
          dd  0x0         ; Will be filled later on

        ; Assume we don't have extended memory
        extendedmem_below16   dw 0
        extendedmem_above16   dw 0

        ; Since we use chars, we define them.. It's smaller than actually
        ; define them as data.
        OKCHAR          equ '.'
        FAILURECHAR     equ 'X'

; --------------------------------
; Start of program
; --------------------------------

Start:
        ; We use 0000:8000 instead of 0800:0000. This is because when we switch to
        ; protected mode, our segemnt descriptor starts at 0x0000 as well.
        xor  ax, ax
        mov  ds, ax

        mov  al, 13	   	    ; Carriage Return
        call WriteChar
        mov  al, 10		      ; Line feed
        call WriteChar


; -------------------------------------------------------
; Detect CPU
DetectCPU:
        ; Check for 8088/86
        pushf                           ; Save the flags

        xor ah,ah
        push ax                         ; Push ax onto stack
        popf                            ; And pop as the flags (which are 0 now)

        pushf                           ; Push '0'-flagregister
        pop ax                          ; Pop it as AX
        cmp ah, 0xF0                    ; Bits 12..15 are set?
        je  DetectCPU_No368Found

        ; Check for 286
        mov  ah,0xF0                    ; Set bits 12..15
        push ax                         ; Push ax and pop as flags
        popf

        pushf                           ; Push flags and pop as ax
        pop  ax
        and  ah, 0xF0                   ; Bits 12..15 are clear?
        jz   DetectCPU_No368Found

        popf                            ; Pop original flags back

DetectCPU_368Found:
        jmp  DetectCPU_End

DetectCPU_No368Found:
        mov  al, FAILURECHAR            ; Uh-oh
        call WriteChar

        call DeadLock                   ; Not good.. DeadLock
        jmp  DetectCPU_End              ; Not used, but nice for synchronosity
DetectCPU_End:


        fninit                          ; Initialise FPU 387


; -------------------------------------------------------
; We need at least 1 MB of memory. If not, we cannot load.
; Our kernel uses the 1st MB of memory for kernel space
; which means only the kernel construct can be used if we
; have only 1 MB of memory for now..

; It should be handled better: we need to find out how much
; memory we have, how much memory the kernel needs (codesize)
; how much memory we need to get up and running (datasize).
DetectMemory:
        push    ds
        mov     si,0x0040
        mov     ds,si
        mov     si,0x0013
        losdw
        pop     ds

        cmp     ax, 1024
        jae     EnoughMemoryFound

NotEnoughMemoryFound:
        mov  al, FAILURECHAR            ; Uh-oh
        call WriteChar
        call DeadLock                   ; Not good.. DeadLock

EnoughMemoryFound:

        ; At least 1024MB was found. Now we are going to check how
        ; much more we have.

        mov     ax, 0xE801
        int     0x15
        jc      SkipExtendedMemory     ; Failure. Assume only 1MB

        mov [extendedmem_below16], ax   ; between 1MB and 16MB in KB
        mov [extendedmem_above16], bx   ; above 16MB in 64KB's

SkipExtendedMemory:

; -------------------------------------------------------
; Set a nifty color palette

        jmp OverPaletteData

        palette db  0,  8,  0, 16     ; Nice blue'ish background color
                db  7, 40, 40, 63     ; And a nice cyan'ish text color
                db -1

OverPaletteData:

        mov  si,palette
NextColor:
        mov  al,[ds:si]
        cmp  al,-1
        je   PaletteDone

        mov dx,0x3C8
        out dx,al

        inc dx
        mov al,[ds:si+1]
        out dx,al
        mov al,[ds:si+2]
        out dx,al
        mov al,[ds:si+3]
        out dx,al

        add si,4
        jmp NextColor
PaletteDone:


; -------------------------------------------------------
; Enable A20 line

EnableA20Line:
        cli                      ; no more interuptions! :)
        xor cx, cx
clear_buf:
        in al, 0x64              ; get input from keyboard status port
        test al, 0x02            ; test the buffer full flag
        loopnz clear_buf         ; loop until buffer is empty

        mov al, 0x0D1            ; keyboard: write to output port
        out 0x64, al             ; output command to keyboard
clear_buf2:
        in al, 0x64              ; wait 'till buffer is empty again
        test al, 0x02
        loopnz clear_buf2

        mov al, 0xdf             ; keyboard: set A20
        out 0x60, al             ; send it to the keyboard controller

        mov cx, 0x14
wait_kbc:                        ; this is approx. a 25uS delay to wait
        out 0edh, ax             ; for the kb controler to execute our
        loop wait_kbc            ; command.
        sti

        ; Dat is fijn.. kunnen we eindelijk wat meer mem aanspreken hierna :) Yeah baby yeah!

; -------------------------------------------------------
; Set up and load GDT
SetupGDT:

        ; Copy the GDT table to correct position, and save this position
        ; into the LGDT_STRUC

        mov   eax, KERNEL_IDT_ADDR     ; First we have a IDT
        add   eax, 256*8               ; we leave some room and that will be the start of our GDT
        mov   [LGDT_STRUC+2], eax      ; Save this address

        shr   eax, 4                   ; get the segment (0x9C800 -> 0x9C80)
        mov   es, ax
        mov   di, 0                    ; gdt always has to start at a offset 0
        mov   si, GDT_TABLE
        mov   cx, gdt_size
        rep   movsd

        cli                             ; Don't use interrupts at this point
        lgdt  [LGDT_STRUC]              ; Load GDT


; -------------------------------------------------------
; Jump To Protected mode

EnterPMode:
        mov  eax,cr0                    ; Set PE bit in cr0
        or   al,1
        mov  cr0,eax                    ; Flags know we're in PM right now, let's make sure the whole CPU knows it:

        jmp ClearPreFetchQueue		; Jump
ClearPreFetchQueue:			; Kaploink. Everything knows we know we're in PM at this point.

        ; Jump to PM_START in our code-descriptor
        jmp dword KERNEL_CODE_SELECTOR:PM_START


; ****************************************************************
; * Routines
; ****************************************************************

; ------------------------------------------------------------------------
; Prints a char to the screen

WriteChar:
        mov  ah,0Eh             ; Print char AL on screen
        mov  bx,0007
        int  0x10               ; Using BIOS routines (Yuk!)
        ret


; -------------------------------------------------------
; Welcome to the deadlock of reality

DeadLock:
        jmp DeadLock



;------------------------------------------------------------
; Everything here is in 32 bits Protected Mode!!!
;------------------------------------------------------------

[bits 32]
        ; This is the entry point inside full 32bits Protected mode. Note that at this point
        ; the registers are still filled with "realmode" values. This includes the stack, so
        ; stack-pushes are done to the "realmode"-stack.

PM_START:
        call    SetupPaging

        ; ---------------------------
        ; Set kernel data selectors
        mov  ax,KERNEL_DATA_SELECTOR
        mov  ds,ax
        mov  es,ax
        mov  fs,ax
        mov  gs,ax

        ; ---------------------------
        ; Set kernel stack selector
        mov  ax,KERNEL_DATA_SELECTOR
        mov  ss,ax
        mov  eax,STACK_PM_ABS
        mov  esp,eax

        ; Calculate memory and push on stack as a parameter for _kernel_entry_point ()
        xor  eax, eax
        mov  ax, [extendedmem_below16]
        shl  eax, 10
        mov  bx, [extendedmem_above16]
        shl  ebx, 16
        add  eax, ebx
        add  eax, 0x100000              ; Add the standard 1 megabyte

        push eax             ; Push parameter 2: int total_sys_memory
        push STACK_PM_ABS    ; Push parameter 1: int stack_start
        push 0xDEADBEEF      ; Dummy return address

        ; ---------------------------
        ; Jump to kernel main.c/_kernel_entry_point() located at the top gigabyte!
        jmp  KERNEL_CODE_SELECTOR:0xC0010000


;****************************************************************
SetupPaging:
        ; We are working in global memory
        mov     ax, KERNEL_DATA_SELECTOR
        mov     ds, ax
        mov     es, ax

        ; Clear page directory
        mov     edi, PAGEDIR
        xor     eax,eax         ; Page dir points to nothing.
        mov     ecx, 1024
        rep stosd

        mov     edi, PAGETABLE_1MB
        xor     eax,eax         ; Page table points to nothing.
        mov     ecx, 1024
        rep stosd


        ; Warning: when adding pagetables, it must be tables aligned on 4Kb marks (eg: 0x1000 0x9F000 etc).
        ; The first 3 bytes should be 0, since we will be using them for flags.

        ; Setup pointer to a page table in the page directory
        mov     eax, PAGETABLE_1MB    ; pointer to a table
        and     eax, 0xFFFFF000       ; clear first 3 bytes
        or      eax, 7                ; Add flags PAGE PRESENT + PAGERW + (NOT_ALL_ACCESS)

        mov     edi, PAGEDIR
        mov     [ds:edi+0x000], eax   ; store in pagetable offset for virtual memory 0x000.....)
        mov     [ds:edi+0xC00], eax   ; store in pagetable offset for virtual memory 0xC00.....)
        mov     [ds:edi+0xF00], eax   ; store in pagetable offset for virtual memory 0xF00.....)



    ; Create page table for first megabyte (1:1 Mapping) This will allow us to function correctly in the loader.

        mov     edi, PAGETABLE_1MB      ; Page table
        mov     ecx, 1024               ; 1024 Pages (* 4kb = 4MB)
        mov     eax, 0x00000007         ; First address in eax, we start with addr 0x00000
                                        ; plus flags PAGE PRESENT + PAGERW + (NOT_ALL_ACCESS)
PageTableLoop1:
        mov     [ds:edi], eax           ; Add to table
        add     eax, 0x00001000         ; next 4Kb (starts at bit 12..31)
        add     edi, 4                  ; Next entry
        loop    PageTableLoop1


        ; Set page directory in cr3 register
        mov     eax, PAGEDIR
        mov     cr3, eax


        ; Enable paging
        mov     eax, cr0
        or      eax, 0x80000000
        mov     cr0, eax

        ret

; -------------------------------------------------------------------------

    ; Filler
    times 1000-($-$$) db 0x90

KRNL_LDR_END:
; Do not add code after this label
