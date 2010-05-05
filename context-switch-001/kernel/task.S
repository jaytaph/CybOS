[bits 32]

section .text

[GLOBAL context_switch]
context_switch:
    pushf
    push    cs
    push    after_iret

    push    byte 0
    push    byte 0

    pusha

    mov   eax, [esp+56]
    mov   [eax], esp        ; save context into previous task

    mov   eax, [esp+60]     ; Set new context
    mov   esp, [eax]

    ; Switch to new stack

    popa
    add  esp, 8             ; remove dummy int en errorcode
    iret

after_iret:
    ret