[bits 32]

section .text

global mutex_bts_asm
global mutex_btr_asm


;************************************************************************
mutex_bts_asm:
        push ebp                ; save ebp
        mov  ebp, esp           ; ebp = base pointer of vars on stack

        mov  eax, [ebp+8]       ; eax = *mutex
        xor  ebx, ebx
        bts  [eax], ebx         ; Test Bit (0) And Set
        jc   BitAlreadySet      ; old bit saved in carry, jump if bit was 1

BitNoSet:
        ; Bit was 0, which means the mutex was not locked by somebody. But at
        ; this point, it is locked.. by us :-)
        mov  eax, 0
        jmp  EndOfFunction

BitAlreadySet:
        ; Bit was 1, which means the mutex is already locked by somebody else
        mov  eax, 1             ; return int, by saving it in eax
        jmp  EndOfFunction

EndOfFunction:
        mov  esp, ebp           ; restore base pointer (in case we changed ESP)
        pop  ebp                ; restore old ebp
        ret


;************************************************************************
mutex_btr_asm:
        push ebp                ; save ebp
        mov  ebp, esp           ; ebp = base pointer of vars on stack

        mov  eax, [ebp+8]       ; eax = mutex_addr
        xor  ebx, ebx
        btr  [eax], ebx         ; Test Bit (0) And ReSet
        jc   BitAlreadyReSet    ; old bit saved in carry, jump if bit was 1

BitNoReSet:
        ; Bit was 0, which means the mutex was not locked by somebody. But at
        ; this point, it is locked.. by us :-)
        mov  eax, 0
        jmp  EndOfFunction2

BitAlreadyReSet:
        ; Bit was 1, which means the mutex is already locked by somebody else
        mov  eax, 1             ; return int, by saving it in eax
        jmp  EndOfFunction2

EndOfFunction2:
        mov  esp, ebp           ; restore base pointer (in case we changed ESP)
        pop  ebp                ; restore old ebp
        ret

