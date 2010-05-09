;***************************************************************
;
;                   BOOT LOADER LEVEL 2
;
;***************************************************************

%include "inc/globals.def"

[bits 16]
[org 0x5000]

        jmp  Start

        ; This block will be filled by the level1 bootloader. Should use pointer
        ; to 7E00 for it actually

        BPB_Name                times 8 db 0            ; equ 0x7E03 + 0 ??
        BPB_BytesPerSector      dw 0
        BPB_SectorsPerCluster   db 0
        BPB_ReservedSectorCount dw 0
        BPB_FatTables           db 0
        BPB_MaxRootDirEntries   dw 0
        BPB_TotalSectors        dw 0
        BPB_MediaDescriptor     db 0
        BPB_SectorsPerFAT       dw 0
        BPB_SectorsPerTrack     dw 0
        BPB_NumberOfHeads       dw 0
        BPB_HiddenSectors       dd 0
        BPB_TotalSectors32      dd 0
        BPB_DriveNumber         db 0
        BPB_Reserved1           db 0
        BPB_BootSignature       db 0
        BPB_VolumeID            dd 0
        BPB_VolumeLabel         times 11 db 0
        BPB_FileType            times 8 db 0

        times BootLvl2SigOffset-($-$$) db 0x0         ; make sure level 2 signature is on correct offset
        dw BootLvl2Signature

; Do not add code before this label
Start:
    jmp  0x0000:StartRelocated          ; At this point we need to be at segment 0, this is because the
                                        ; GDT is 0-based and cannot find data when pmode is entered.
StartRelocated:
    push cs                             ; Set DS to CS
    pop  ds

    mov  [BootDriveID], dl              ; Save boot drive

    %include "level2/preload.S"     ; Pre loading stuff
	%include "level2/loader.S"      ; FAT12 loader of the kernel
	%include "level2/pmode.S"       ; Protected mode entering code
    ; NOTE: PMODE.S JUMPS TO KERNEL


; The next code is 16bit
[bits 16]

; ------------------------------------------------------------------------
; Writes a message on screen
;  In: DS:SI -> Message offset
WriteMsg:
        lodsb                           ; Get char and point to the next
        cmp  al, 0
        je   .done                      ; Yes, break the loop

        mov  ah,0Eh                     ; Print char AL on screen
        mov  bx,0x0007
        int  0x10                       ; Using BIOS routines

        jmp  WriteMsg                   ; And loop
.done:
        ret


; ------------------------------------------------------------------------
; Prints error and deadlocks the system
;  In: DS:SI -> Error message
BootError:
        call WriteMsg
        jmp $



BOOTLVL2_END:
; Do not add code after this label
