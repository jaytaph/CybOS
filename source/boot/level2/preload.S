;********************************************************
;*
;* CybOS Boot Sector - Level 2
;*   Preload stuff
;*
;********************************************************

; -------------------------------------------------------
; Set a nifty color palette

        jmp SOC_Preloader

        ErrorStr_386NotFound   db 13,10,'Need at least a 386 processor to run.',0
        ErrorStr_1MBNotFound   db 13,10,'Need at least 1MB of memory to run.',0

        paletteData db  0,  8,  0, 16     ; Nice blue'ish background color
                    db  7, 40, 40, 63     ; And a nice cyan'ish text color
                    db -1

SOC_Preloader:

; -------------------------------------------------------
; We need at least 1 MB of memory. If not, we cannot load.
; Our kernel uses the 1st MB of memory for kernel space
; which means only the kernel construct can be used if we
; have only 1 MB of memory for now..
;
; It should be handled better: we need to find out how much
; memory we have, how much memory the kernel needs (codesize)
; how much memory we need to get up and running (datasize).
DetectMemory:
        ; Assume 1MB found. Now we are going to check how
        ; much more we have (not really a good test, but sufficient for now)

        mov     ax, 0xE801
        int     0x15
        jc      .skipExtendedMemory      ; Failure. Assume only 1MB

        mov [extendedmem_below16], ax   ; between 1MB and 16MB in KB
        mov [extendedmem_above16], bx   ; above 16MB in 64KB's
.skipExtendedMemory:


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
        je  .detectCPU_No368Found

        ; Check for 286
        mov  ah,0xF0                    ; Set bits 12..15
        push ax                         ; Push ax and pop as flags
        popf

        pushf                           ; Push flags and pop as ax
        pop  ax
        and  ah, 0xF0                   ; Bits 12..15 are clear?
        jz   .detectCPU_No368Found

        popf                            ; Pop original flags back

.detectCPU_368Found:
        jmp  .detectCPU_End

.detectCPU_No368Found:
        mov  si, ErrorStr_386NotFound
        call WriteMsg
        jmp  $                          ; Not good.. DeadLock

.detectCPU_End:


; -------------------------------------------------------
; Init FPU
        fninit                          ; Initialise FPU 387


; -------------------------------------------------------
; Set color palette
        mov  si,paletteData
.nextColor:
        lodsb
        cmp  al,-1
        je   PaletteDone

        mov dx,0x3C8            ; Set color index
        out dx,al

        inc dx                  ; 0x3C9 is data port
        lodsb
        out dx,al               ; Set R component
        lodsb
        out dx,al               ; set G component
        lodsb
        out dx,al               ; Set B component

        jmp .nextColor
PaletteDone: