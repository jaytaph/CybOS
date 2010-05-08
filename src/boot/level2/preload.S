;********************************************************
;*
;* CybOS Boot Sector - Level 2
;*   Preload stuff
;*
;********************************************************

; -------------------------------------------------------
; Set a nifty color palette

        jmp SOC_Preloader

        paletteData db  0,  8,  0, 16     ; Nice blue'ish background color
                    db  7, 40, 40, 63     ; And a nice cyan'ish text color
                    db -1

SOC_Preloader:
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