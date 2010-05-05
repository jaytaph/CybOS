TODO

        CHECK IF DIRECTORY LOADING FROM DIFFERENT CLUSTERS WORK (WHEN KERNEL.BIN IS IN SECOND CLUSTER)
        CREATE LOADFILE
        DONE

;********************************************************
;*
;* CybOS Boot Sector - Level 2
;*   Kernel loader
;*
;********************************************************

        jmp SOC_Loader                          ; Jump over the data

        BootLvl2Dir     db 'SYSTEM     '            ; Directory containing system files
        KernelFile      db 'KERNEL  BIN'            ; Kernel file

        BootDriveID     db 0                        ; Holds the current boot drive ID

        LBA_S           db 0                        ; Set when converting LBA2CHS()
        LBA_H           db 0
        LBA_C           db 0

        FATStartLo      dw 0                        ; FAT Issues
        FATStartHi      dw 0
        RootDirStartLo  dw 0
        RootDirStartHi  dw 0
        DataStartLo     dw 0
        DataStartHi     dw 0

        DirectoryEntry  times 32 db 0           ; Temporary directory entry

        ; Data defines

        TMP_DIR_SEG     equ 0x4000              ; Temporary directory to store FAT data
        TMP_DIR_OFF     equ 0x0000

        ATTR_READONLY   equ 0x01                ; FAT 12 file attributes
        ATTR_HIDDEN     equ 0x02
        ATTR_SYSTEM     equ 0x04
        ATTR_VOLLABEL   equ 0x08
        ATTR_SUBDIR     equ 0x10
        ATTR_ARCHIVE    equ 0x20
        ATTR_DEVICE     equ 0x40
        ATTR_UNUSED     equ 0x80

        ErrorReading      db 13,10,'Error while reading floppy...',0
        ErrorNoSystemDir  db 13,10,'Cannot find /SYSTEM directory...',0
        ErrorNoKernelFile db 13,10,'Cannot find /SYSTEM/KERNEL.BIN...',0

; -------------------------------------------------------
; Start of code
SOC_Loader:
        mov  ax, cs                             ; Set correct DS and ES segments
        mov  ds, ax
        mov  es, ax

        mov  [BootDriveID], dl

; -------------------------------------------------------
; Init FAT stuff
        call InitFAT12

; -------------------------------------------------------
; Find system directory from rootdir
        mov  si, BootLvl2Dir                    ; Seek SYSTEM directory
        call FindFromRootDir

        cmp  bx, 0
        jne   .NoBootError1
        mov  si, ErrorNoSystemDir
        call BootError
.NoBootError1:

; -------------------------------------------------------
; Find and load boot level2 file
        ; AX is already set by FindDir
        mov  si, KernelFile                   ; Load KERNEL.BIN in SYSTEM root directory
        mov  di, DirectoryEntry               ; Place to store directory entry data
        call FindFile
        cmp  bx, 0
        jne  .NoBootError2
        mov  si, ErrorNoKernelFile
        call BootError
.NoBootError2:

        mov  di, DirectoryEntry
        mov  bx, KERNEL_SEG
        mov  es, bx
        mov  bx, KERNEL_OFF
        call LoadFile

; -------------------------------------------------------
; All done here
        jmp EOF_Loader


; ****************************************************************
; * Routines
; ****************************************************************

; ------------------------------------------------------------------------
; Reads sectors into memory
;  In: DX:AX -> 32bit LBA start sector
;      CL    -> Sector count
;      ES:BX -> target memory block
ReadSectors:
        mov  di, 5                      ; Retry 5 times at most
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

        mov  si, ErrorReading
        call BootError                  ; Not good, error

SectorReadOK:                           ; Everything went as expected
        ret


; ------------------------------------------------------------------------
; Convert LBA address into CHS address
;  In:  DX:AX -> LBA 32bit
;  Out: <none>  (LBA_C,LBA_H and LBA_S are set)
LBA2CHS:
        pusha                                           ; Save all regs

        mov  bx, [BPB_SectorsPerTrack]
        div  bx
        inc  dx                                         ; Sector is 1-based

        and  dx, 0x3F
        mov  [LBA_S], dx

        xor  dx, dx
        mov  bx, [BPB_NumberOfHeads]
        div  bx

        mov  [LBA_H], dl
        mov  [LBA_C], al

        clc                                             ; Bit 8 and 9 are in sector field
        shl  ah, 6
        or   [LBA_S], ah

        popa
        ret


; ------------------------------------------------------
InitFAT12:
        xor  ax, ax                              ; Set high values for start sectors to 0
        mov  [RootDirStartHi], ax
        mov  [DataStartHi], ax

        add  ax, [BPB_HiddenSectors]
        adc  dx, [BPB_HiddenSectors+2]
        add  ax, [BPB_ReservedSectorCount]
        adc  dx, 0
        mov  [FATStartHi], dx
        mov  [FATStartLo], ax

        mov  al, [BPB_FatTables]                 ; Calculate root and data start sectors
        mul  word [BPB_SectorsPerFAT]
        add  ax, [BPB_HiddenSectors]
        adc  dx, [BPB_HiddenSectors+2]
        add  ax, [BPB_ReservedSectorCount]
        adc  dx, 0
        mov  [RootDirStartLo], ax
        mov  [DataStartLo], ax
        mov  [RootDirStartHi], dx
        mov  [DataStartHi], dx


        mov  ax, 32                              ; Calculate data start sector
        mov  bx, [BPB_MaxRootDirEntries]
        mul  bx
        mov  bx, [BPB_BytesPerSector]
        add  ax, bx
        dec  ax
        div  bx
        add  [DataStartLo], ax
        adc  word [DataStartHi], 0
        ret


; ------------------------------------------------------
; Find a FAT12 directory in the FAT root directory entry
;   In:  DS:SI -> Directory name
;   Out: AX    -> Start cluster
;        BX    -> status (1 = found, 0 = not found)
FindFromRootDir:

; Read all directory entries
        mov  cx, [BPB_MaxRootDirEntries]         ; Find how many sectors rootdir entry is
        dec  cx
        shr  cx, 4
        inc  cx                                  ; Must be in CX

        mov  dx, [RootDirStartHi]
        mov  ax, [RootDirStartLo]
        mov  bx, TMP_DIR_SEG
        mov  es, bx
        mov  bx, TMP_DIR_OFF
        call ReadSectors

; Now, scan the directory entries for correct dirname
        mov  di, TMP_DIR_OFF
        mov  cx, [BPB_MaxRootDirEntries]         ; Loop this many root dir entries (max)
ReadDirEntry:
        mov   bx, cx                             ; Save current counter
        cld                                      ; Compare ds:si to es:di (11 char)
        mov   cx, 11
        push  di
        push  si
        rep   cmpsb
        pop   si
        pop   di
        jne   .NotFound
.Found:
        mov   al, [es:di+11]                     ; See if this file (SYSTEM) is a directory
        and   al, ATTR_SUBDIR
        jz    .NotFound

        ; Set return values
        mov   ax, [es:di+26]
        mov   bx, 1
        ret
.NotFound:
        mov   cx, bx                             ; Restore counter
        add   di, 32
        loop  ReadDirEntry

        xor  bx, bx                              ; Directory not found
        ret


; ------------------------------------------------------
; Find a file in specified directory block
;  In:  DS:SI -> file
;       DI    -> DirectoryEntry storage (not filled)
;       AX    -> Start cluster of directory
;  Out: BX    -> Status: 0 not found, 1 found (DI filled)

filePtr dw 0
dirStorage dw 0
curCluster dw 0

FindFile:
        mov  [curCluster], ax
        mov  [dirStorage], di
        mov  [filePtr], si

.NextCluster:
        mov  ax, [curCluster]                     ; Read cluster with directory info
        call ConvertClusterToLBA

        xor  cx, cx
        mov  cl, [BPB_SectorsPerCluster]
        mov  bx, TMP_DIR_SEG
        mov  es, bx
        mov  bx, TMP_DIR_OFF
        call ReadSectors

XCHG BX,BX
        mov  cx, [BPB_SectorsPerCluster]
        shr  cx, 4                               ; Div 32
        mov  di, TMP_DIR_OFF
.scanDirectoryCluster:
        push cx

        xor  al, al                              ; First index of data entry is 0, no more entries
        cmp  [es:di], al
        je   .NotFound

        ; Compare filename (+extension)
        cld
        mov  cx, 11
        mov  si, [filePtr]
        push di
        rep  cmpsb
        pop  di
        jne  .NextEntry
.Found:
        add  sp, 2                               ; Remove unused loop CX
        mov  si, di                              ; Found data, copy directory entry
        mov  di, [dirStorage]
        mov  cx, 32
        rep  movsb
        mov  bx, 1
        ret
.NextEntry:
        add  di, 32
        pop  cx
        loop .scanDirectoryCluster

        ; Done with this cluster. Read another cluster
        mov  ax, [curCluster]
        call getNextFATCluster

        ; Check if next cluster is valid (or no more clusters)
        cmp  ax, 0x002
        jb   .NotFound
        cmp  ax, 0xFF7
        ja   .NotFound

        jmp  .NextCluster


.NotFound:
        xor  bx, bx
        ret


; ------------------------------------------------------
; In  : AX -> cluster number
; Out : AX -> next cluster number
getNextFATCluster:
    mov  bx, ax
    shr  bx, 1
    add  ax, bx

    mov  si, FATStartLo
    add  si, ax
    lodsw

    and  ax, 1
    jne  .even
.odd:
    and  ax, 0x0FFF;
    jmp  .done
.even:
    shl  ax, 4
    jmp  .done
.done
    ret

; ------------------------------------------------------
; In  : AX -> cluster number
; Out : DX:AX -> LBA sector
ConvertClusterToLBA:
    mov   bx, 1
    dec   ax
    dec   ax
    mov   bl, [BPB_SectorsPerCluster]
    xor   bh, bh
    mul   bx

    mov   bx, 1
    add   ax, [DataStartLo]               ; DX:AX is filled
    adc   dx, [DataStartHi]
    ret

; ------------------------------------------------------
; Find a file in specified directory block
;  In:  DS:SI -> FAT12 Directory entry
;       ES:BX -> File destination
LoadFile:
        ret



; Do not add code after this label
EOF_Loader:



