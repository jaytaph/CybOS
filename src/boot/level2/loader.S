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

        LBA_FATStartLo      dw 0                    ; FAT (LBA) sector starts
        LBA_FATStartHi      dw 0
        LBA_RootDirStartLo  dw 0
        LBA_RootDirStartHi  dw 0
        LBA_DataStartLo     dw 0
        LBA_DataStartHi     dw 0

        DirectoryEntry  times 32 db 0           ; Temporary directory entry

        ; Data defines
        TMP_DIR_SEG     equ 0x4000              ; Temporary directory to store temporary cluster data
        TMP_DIR_OFF     equ 0x0000
        FAT_DIR_SEG     equ 0x4000              ; Temporary directory to store FAT data
        FAT_DIR_OFF     equ 0x8000

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
        push ds
        pop  es

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
; Find kernel file's directoy entry
        ; AX (LBA of directory) is already set by FindDir
        mov  si, KernelFile                   ; Load KERNEL.BIN in SYSTEM root directory
        mov  di, DirectoryEntry               ; Place to store directory entry data
        push ds
        pop  es
        call FindFile
        cmp  bx, 0
        jne  .NoBootError2
        mov  si, ErrorNoKernelFile
        call BootError
.NoBootError2:

; -------------------------------------------------------
; Load actual kernel file
        mov  si, DirectoryEntry
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
        call LBA2CHS

        mov  di, 5                      ; Retry 5 times at most
RS_Retry:

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
        xor  dx, dx
        mov  [LBA_RootDirStartHi], ax
        mov  [LBA_DataStartHi], ax

        add  ax, [BPB_HiddenSectors]
        adc  dx, [BPB_HiddenSectors+2]
        add  ax, [BPB_ReservedSectorCount]
        adc  dx, 0
        mov  [LBA_FATStartHi], dx
        mov  [LBA_FATStartLo], ax

        mov  al, [BPB_FatTables]                 ; Calculate root and data start sectors
        mul  word [BPB_SectorsPerFAT]
        add  ax, [BPB_HiddenSectors]
        adc  dx, [BPB_HiddenSectors+2]
        add  ax, [BPB_ReservedSectorCount]
        adc  dx, 0
        mov  [LBA_RootDirStartLo], ax
        mov  [LBA_DataStartLo], ax
        mov  [LBA_RootDirStartHi], dx
        mov  [LBA_DataStartHi], dx

        mov  ax, 32                              ; Calculate data start sector
        mov  bx, [BPB_MaxRootDirEntries]
        mul  bx
        mov  bx, [BPB_BytesPerSector]
        add  ax, bx
        dec  ax
        div  bx
        add  [LBA_DataStartLo], ax
        adc  word [LBA_DataStartHi], 0

        ; Read FAT entries
        mov  cx, [BPB_SectorsPerFAT]
        mov  ax, [LBA_FATStartLo]
        mov  dx, [LBA_FATStartHi]
        mov  bx, FAT_DIR_SEG
        mov  es, bx
        mov  bx, FAT_DIR_OFF
        call ReadSectors

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

        mov  dx, [LBA_RootDirStartHi]
        mov  ax, [LBA_RootDirStartLo]
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
;       DS:DI -> DirectoryEntry storage (not filled)
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
        je  .Found
.NextEntry:
        add  di, 32
        pop  cx
        loop .scanDirectoryCluster

        ; Done with this cluster. Read another cluster
        mov  ax, [curCluster]
        call getNextFATCluster
        mov  [curCluster], ax

        ; Check if next cluster is valid (or no more clusters)
        cmp  ax, 0x002
        jb   .NotFound
        cmp  ax, 0xFF7
        ja   .NotFound

        jmp  .NextCluster

.Found:
        add  sp, 2                               ; Remove unused loop CX

        mov  si, di                              ; Found data, copy directory entry
        mov  di, [dirStorage]

        mov  ax, es
        mov  bx, ds
        mov  es, bx
        mov  ds, ax

        mov  cx, 32
        rep  movsb

        mov  ax, es
        mov  bx, ds
        mov  es, bx
        mov  ds, ax

        mov  bx, 1
        ret

.NotFound:
        xor  bx, bx
        ret


; ------------------------------------------------------
; In  : AX -> cluster number
; Out : AX -> next cluster number
getNextFATCluster:
    push ds
    mov  cx, ax                         ; Save original cluster

    mov  bx, FAT_DIR_SEG                ; We are browsing FAT entries
    mov  ds, bx

    mov  bx, ax                         ; Calculate correct FAT entry offset (cluster * 2 / 3)
    shr  bx, 1
    add  ax, bx

    mov  si, FAT_DIR_OFF                ; Load FAT entry (word)
    add  si, ax
    lodsw

    test cx, 1                          ; It's 12 byte, test odd/even cluster number
    jne  .even
.odd:
    and  ax, 0x0FFF                     ; Even, mask first 12 bytes
    jmp  .done
.even:
    shr  ax, 4                          ; Bits 4-15 become 0-11
    and  ax, 0x0FFF                      ; just in case something else was present (carry flag)
    jmp  .done

.done
    pop  ds
    ret

; ------------------------------------------------------
; In  : AX -> cluster number
; Out : DX:AX -> LBA sector
ConvertClusterToLBA:
    dec   ax                                  ; Sector start is -2
    dec   ax
    xor   bx, bx                              ; This many sectors per cluster
    mov   bl, [BPB_SectorsPerCluster]
    mul   bx

    add   ax, [LBA_DataStartLo]               ; LBA sector is RELATIVE from DataStart LBA sector.
    adc   dx, [LBA_DataStartHi]
    ret

; ------------------------------------------------------
; Find a file in specified directory block
;  In:  DS:SI -> FAT12 Directory entry
;       ES:BX -> File destination
LoadFile:
        mov  ax, [si + 0x1A]                      ; Points to firstCluster
        mov  [curCluster], ax
        mov  [filePtr], bx

.NextCluster:
        mov  ax, [curCluster]                     ; Read cluster with directory info
        call ConvertClusterToLBA

        xor  cx, cx                               ; Read X number of sectors (1 cluster in total)
        mov  cl, [BPB_SectorsPerCluster]
        mov  bx, [filePtr]
        call ReadSectors

        mov  ax, [BPB_BytesPerSector]             ; Add byte offset to fileptr
        xor  bx, bx
        mov  bl, [BPB_SectorsPerCluster]
        mul bx
        add  [filePtr], ax
        jno  .NoOffsetOverFlow
.OffsetOverflow:                                  ; Offset overflow, increase segment.
        mov  ax, es
        add  ax, 16                               ; 0400:0F00 (4FF00 becomes 0400:0100)
        mov  es, ax

.NoOffsetOverFlow:
        ; Done with this cluster. Read another cluster
        mov  ax, [curCluster]
        call getNextFATCluster
        mov  [curCluster], ax

        ; Check if next cluster is valid (or no more clusters)
        cmp  ax, 0x002
        jb   .Done
        cmp  ax, 0xFF7
        ja   .Done

        jmp  .NextCluster

.Done:
        ret

; -----------------------------------------------
; Do not add code after this label
EOF_Loader:



