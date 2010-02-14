;********************************************************
;*
;* CybOS Boot Sector
;*
;********************************************************

[bits 16]                       ; BIOS starts in 16 bit.
[org 0]                         ; Relative from offset 0

; -------------------------------------------------------
; Move bootsector to another place so we can load the
; bootsector of a harddisk (instead of the MBR).

        mov  ax,0x7C0                   ; ds  07C0h
        mov  ds,ax
        xor  si,si
        mov  ax,BOOT_SEG                ; es:di = 07E0:0000
        mov  es,ax
        mov  di,BOOT_OFF
        mov  cx,512                     ; Move the bootsector to 07E0:0000
        rep  movsb

        jmp  BOOT_SEG:RelocatedBootBlock    ; Jump to correct address (07E0 segment)


; --------------------------------
; Data used in the bootsector
; --------------------------------

        BootMsg         db 13,10,'Loading CybOS V0.01... ',0

        BootDriveID     db 0
        CurS            db 0    ; Current Sector
        CurH            db 0    ; Current Head
        CurC            dw 0    ; Current Cylinder
        MaxS            db 0    ; Maximum Sector
        MaxH            db 0    ; Maximum Head
        Cur_Off         dw 0
        Cur_Seg         dw 0


; --------------------------------
; Defines, we do not declare as data
; since it only takes up space
; --------------------------------

%include "globals.def"

        ; Since we use chars, we define them.. It's smaller than actually
        ; define them as data.
        OKCHAR          equ '.'
        FAILURECHAR     equ 'X'



; -------------------------------------------------------
; We're here at: 07E0h:0000h

RelocatedBootBlock:
        mov  ax,BOOT_SEG                ; DS = segment so we still can use ORG 0
        mov  ds,ax

; -------------------------------------------------------
; Save bootdrive

        mov  [BootDriveID],dl

; -------------------------------------------------------
; Show bootmessage

        mov  si,BootMsg                 ; Load bootmsg
        call WriteMsg

; -------------------------------------------------------
; Setup stack space
        cli                             ; Clear Ints
        mov  ax,STACK_RM_SEG            ; Set temporary stack to 0900 segment
        mov  ss,ax
        mov  sp,STACK_RM_SIZE
        sti                             ; Restore Ints

; -------------------------------------------------------
; Set screenmode
;        mov  ax,0x1112                  ; Set a decent screenmode. Better change when you really want people
;        mov  bl,0                       ; to use it on less than VGA/MCGA
;        int  0x10

; -------------------------------------------------------
; Get drive information

        xor  ax,ax			                ; Reset floppy disk
        mov  dl,[BootDriveID]
        int  0x13

        xor  di,di			                ; Do not fill drive params
        mov  es,di
        mov  ah,8
        mov  dl,[BootDriveID]
        int  0x13

        ;------------------------------------------
        and  cl,00111111b
        mov  [MaxS],cl                  ; Maximum Sectors
        mov  [MaxH],dh                  ; Maximum Heads

        mov  dl,[BootDriveID]
        xor  ah,ah                      ; Reset boot drive
        int  0x13


; -------------------------------------------------------
; Read kernel sectors into memory. The first sector of
; our kernel says how many kernel sectors there are.

        mov  bx,KRNL_LDR_SEG            ; Set destination segment
        mov  [Cur_Seg],bx
        mov  bx,KRNL_LDR_OFF
        mov  [Cur_Off],bx

        xor  ax,ax
        mov  [CurH],al
        mov  [CurC],ax
        mov  al,2
        mov  [CurS],al
        call ReadSector                 ; Read 1st kernel_loader sector

        mov  ax, KRNL_LDR_SEG
        mov  es,ax
        mov  al, [es:KRNL_LDR_OFF+10]   ; Offset 10 of kernel tells us how many sectors we need to load.

KernelLoadLoop:
        dec  al
        jz   KernelLoadDone
        push ax                         ; save the sector counter
        call ReadSector                 ; all registers are still ok. Load the whole kernel.
        pop  ax                         ; restore the sector counter
        jmp  KernelLoadLoop
KernelLoadDone:


        ; Not good.. must be done by the kernel_loader.. but since the
        ; kernel is still raw data and not on a filesystem, load it here..

        mov  bx,KERNEL_SEG            ; Set destination segment
        mov  [Cur_Seg],bx
        mov  bx,KERNEL_OFF
        mov  [Cur_Off],bx

        mov  ax,100                     ; Load this many kernel sectors
RealKernelLoadLoop:
        push ax                         ; save the sector counter
        call ReadSector                 ; all registers are still ok. Load the whole kernel.
        pop  ax                         ; restore the sector counter
        dec  ax                         ; Just read... doesn't matter if we read to much sectors...
        jnz  RealKernelLoadLoop
RealKernelLoadDone:


        jmp  KRNL_LDR_SEG:KRNL_LDR_OFF  ; Jump to entry of the kernel_loader


; ****************************************************************
; **
; ** Routines
; **
; ****************************************************************

; -------------------------------------------------------
; CurC,CurH,CurS,BootDriveID,Cur_Seg and
; Cur_Off should be filled

ReadSector:
        mov  di,5                ; 5 times retry before failure
RS_Retry:
        mov  bx,[Cur_Seg]
        mov  es,bx
        mov  bx,[Cur_Off]

        mov  dl,[BootDriveID]
        mov  dh,[CurH]
        mov  ch,[CurC]
        mov  cl,[CurS]
        mov  ax,0x0201           ; Read 1 Sector Function
        int  0x13
        jnc  RS_GoodLoad        ; Was it as good for you as it was for me?

;        mov  al, 'X'            ; Uh-oh.. 5 times and still no correct load
;        call WriteChar

        dec  di                 ; No, decrease di
        jnz  RS_Retry           ; Tried it already 5 times?

        mov  al, FAILURECHAR    ; Uh-oh.. 5 times and still no correct load
        call WriteChar
        jmp  $                  ; DeadLock!

RS_GoodLoad:
;        mov  al, OKCHAR         ; Alrighty. Sector is loaded.
;        call WriteChar

        ; -----------------------------------
        ; Set pointer to next memory location
        mov  bx,[Cur_Off]
        add  bx,512
        cmp  bx, 0
        jnz  NoSegmentIncrease
SegmentIncrease:
        mov  ax,[Cur_Seg]       ; Add 0x1000 to the segment (so we can load
        add  ax,0x1000          ; safely trough the 64K boundary of a segment
        mov  [Cur_Seg],ax       ; We're not responsible for segment overflow though.
NoSegmentIncrease:
        mov  [Cur_Off],bx

        ; --------------------------------
        ; Increase Sector/Head/Cylinder
        mov  dl,[BootDriveID]
        mov  dh,[CurH]
        mov  ch,[CurC]
        mov  cl,[CurS]
        call IncreaseCHS
        mov  [CurH],dh
        mov  [CurC],ch
        mov  [CurS],cl

        ret

; ------------------------------------------------------------------------
IncreaseCHS:
        mov  al, cl
        and  cl, 00111111b
        and  al, 11000000b

        inc  cl
        cmp  cl, [MaxS]
        jna  SectorStillOK

        mov  cl, 1
        inc  dh

        cmp  dh, [MaxH]
        jna  HeadStillOK

        mov  dh, 0
        inc  ch
        jno  NoCylinderOverflow

        add  al,01000000b

NoCylinderOverflow:
HeadStillOK:
SectorStillOK:
        or  cl,al
        ret



; ------------------------------------------------------------------------
; Prints a char to the screen

WriteChar:
        mov  ah,0Eh             ; Print char AL on screen
        mov  bx,0x0007
        int  0x10               ; Using BIOS routines (Yuk!)
        ret

; -------------------------------------------------------
; Writes a message from DS:SI
WriteMsg:
        lodsb                           ; Get char and point to the next
        cmp  al,0
        jz   EndWriteMsg                ; Yes, break the loop
        call WriteChar                  ; Nope, print the char
        jmp  WriteMsg                   ; And loop
EndWriteMsg:
        ret


; -------------------------------------------------------
; Write AL into hex AX
MakeHex:
        mov cl,0x10
        mov ah,0
        div cl
        add al,0x30
        add ah,0x30
        cmp al,'9'
        jbe .ca4
        add al,7
.ca4:
        cmp ah,'9'
        jbe .ca5
        add ah,7
.ca5:
        ret

;--------------------------------------------------------------------------------

        times 510-($-$$) db 0

        ; Boot sector must be closed with 0xAA55
        dw 0xAA55
