;********************************************************
;*
;* CybOS Boot Sector
;*
;********************************************************

%include "inc/globals.def"

[bits 16]                       ; BIOS starts in 16 bit.
[org 0]                         ; Relative from offset 0

        jmp StartOfBoot

; Disk Parameter block
BPB_Start:
        BPB_Name                db 'CYBOS   '
        BPB_BytesPerSector      dw 512
        BPB_SectorsPerCluster   db 1
        BPB_ReservedSectorCount dw 1
        BPB_FatTables           db 2
        BPB_MaxRootDirEntries   dw 224
        BPB_TotalSectors        dw 2880
        BPB_MediaDescriptor     db 0xF0     ; 1.44MB
        BPB_SectorsPerFAT       dw 9
        BPB_SectorsPerTrack     dw 18
        BPB_NumberOfHeads       dw 2
        BPB_HiddenSectors       dd 0
        BPB_TotalSectors32      dd 0
        BPB_DriveNumber         db 0
        BPB_Reserved1           db 0
        BPB_BootSignature       db 0x29
        BPB_VolumeID            dd 0xDEADBEEF
        BPB_VolumeLabel         db 'CYBOS      '
        BPB_FileType            db 'FAT12   '

; --------------------------------
; Data used in the bootsector
; --------------------------------


        ; IMPORTANT. THESE BYTES ARE SET WHEN CREATING A BOOTDRIVE. SINCE 512 BYTES IS NOT
        ; ENOUGH FOR US TO ACTUALLY CREATE A FAT12 PARSER, WE NEED SECTOR START AND LENGTH
        ; FOR US TO FIND BOOTLOADER LEVEL2.
        ; THE ONLY THING THIS BOOTSECTOR SHOULD DO IS LOAD AND RUN IT..

        BootLevel2Sectors:
            dd 0x24           ; LBA sector 36
            times 4 dd 0x00   ; room for 4 more sectors. If boot level 2 is more than 2048
                              ; bytes, we need to load the rest of the sectors in the start
                              ; of bootloader level 2

            dd 0x00           ; Final End of sector marker
        nop

        BootMsg            db 13,10,'Loading CybOS V0.02... ',0
        ErrorLvl2NotFound  db 13,10,'Cannot find BOOTLVL2.BIN :(',0
        Error386NotFound   db 13,10,'Need at least a 386 processor to run.',0

        BootDriveID     db 0     ; Holds the current boot drive ID

        LBA_S           db 0
        LBA_H           db 0
        LBA_C           db 0

; -------------------------------------------------------
; Move bootsector to another place so we can load the
; bootsector of a harddisk (instead of the MBR).

StartOfBoot:
        mov  ax,0x7C0                   ; ds  07C0h
        mov  ds,ax
        xor  si,si
        mov  ax,BOOT_LEVEL_1_SEG        ; es:di = 07E0:0000
        mov  es,ax
        mov  di,BOOT_LEVEL_1_OFF
        mov  cx,512                     ; Move the bootsector to 07E0:0000
        rep  movsb

        jmp  BOOT_LEVEL_1_SEG:RelocatedBootBlock    ; Jump to correct address (07E0 segment)

; -------------------------------------------------------
; We're here at: 07E0h:0000h
RelocatedBootBlock:
        mov  ax,BOOT_LEVEL_1_SEG        ; DS = segment so we still can use ORG 0
        mov  ds,ax

; -------------------------------------------------------
; Save bootdrive
        mov  [BootDriveID],dl

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
        mov  si, Error386NotFound
        call WriteMsg
        jmp  $                          ; Not good.. DeadLock

DetectCPU_End:


; -------------------------------------------------------
; Init FPU
        fninit                          ; Initialise FPU 387



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
; Reset FD
        xor  ax,ax			            ; Reset floppy disk
        mov  dl,[BootDriveID]
        int  0x13

; -------------------------------------------------------
; Find system directory from rootdir
        mov  bx, BOOT_LEVEL_2_SEG
        mov  es, bx
        mov  bx, BOOT_LEVEL_2_OFF

        mov  bp, BootLevel2Sectors     ; BP holds pointert to sectors
.nextSectorRead:
        mov  dx, [bp+2]
        mov  ax, [bp]
        add  bp, 4

        cmp  dx, 0                      ; When both AX and DX are 0, stop reading
        jne  .ok
        cmp  ax, 0
        jne  .ok
        jmp  .done                      ; Both 0, end of read
.ok:
        mov  cx, 1                      ; Ok to read 1 (LBA) sector
        call ReadSectors
        jmp  .nextSectorRead

.done:
        ; TODO: Compare signature of level2 boot loader

; -------------------------------------------------------
; Copy BPB into level2 boot loader
       mov  si, BPB_Start
       mov  di, BOOT_LEVEL_2_OFF+3

       mov  cx, 0x36
       rep  movsb


; -------------------------------------------------------
; Jump to boot level 2
        mov  dl, [BootDriveID]
        jmp  BOOT_LEVEL_2_SEG:BOOT_LEVEL_2_OFF           ; Jump to entry of the boot level 2 loader


; ****************************************************************
; **
; ** Routines
; **
; ****************************************************************

; ------------------------------------------------------------------------
; Reads sectors into memory
;  In: DX:AX -> 32bit LBA start sector
;      CL    -> Sector count
;      ES:BX -> target memory block
ReadSectors:
        mov  di, 5
RS_Retry:
        call LBA2CHS

        mov  al, cl
        mov  ah, 0x02
        mov  cl, [LBA_S]
        mov  ch, [LBA_C]
        mov  dl, [BootDriveID]
        mov  dh, [LBA_H]
        int  0x13
        jnc   SectorReadOK              ; Ok read?

        dec  di                         ; Retry a number of times,
        jnz  RS_Retry

        jmp  BootError                  ; Not good, error

SectorReadOK:                           ; Everything went as expected
        ret


; ------------------------------------------------------------------------
; Convert LBA address into CHS address
;  In:  DX:AX -> LBA 32bit
;  Out: <none>  (LBA_C,LBA_H and LBA_S are set)
LBA2CHS:
        pusha

        mov  bx, [BPB_SectorsPerTrack]
        div  bx
        inc  dx         ; Sector is 1-based

        and  dx, 0x3F
        mov  [LBA_S], dx

        xor  dx, dx
        mov  bx, [BPB_NumberOfHeads]
        div  bx

        mov  [LBA_H], dl
        mov  [LBA_C], al

        clc
        shl  ah, 6
        or   [LBA_S], ah

        popa
        ret


; ------------------------------------------------------------------------
; Prints error and does not return
BootError:
        mov  si, ErrorLvl2NotFound
        call WriteMsg
        jmp  $

; ------------------------------------------------------------------------
; Writes a message on screen
;  In: DS:SI -> Message offset
WriteMsg:
        lodsb                           ; Get char and point to the next
        cmp  al,0
        jz   EndWriteMsg                ; Yes, break the loop

        mov  ah,0Eh                     ; Print char AL on screen
        mov  bx,0x0007
        int  0x10                       ; Using BIOS routines

        jmp  WriteMsg                   ; And loop
EndWriteMsg:
        ret

;--------------------------------------------------------------------------------

        times 510-($-$$) db 0x90

        ; Boot sector must be closed with 0xAA55
        dw 0xAA55
