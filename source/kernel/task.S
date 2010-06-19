[bits 32]

section .text

[GLOBAL read_eip]
read_eip:
  pop eax
  jmp eax


[GLOBAL context_switch]
context_switch:
    pushf                   ; NOTE: When changing stack order, you must change
    push    cs              ; the offsets 56 and 60 below as well.
    push    after_iret

    push    byte 0
    push    byte 0

    pusha

    mov   eax, [esp+56]     ; 56 = regs_t previous_task->esp0
    mov   [eax], esp        ; save esp into previous task

    mov   eax, [esp+60]     ; Set esp of new task
    mov   esp, [eax]        ; 60 = regs_t previous_task->esp0

    ; Stack is now switched

    popa
    add  esp, 8             ; remove dummy int en errorcode
    iret                    ; This IRET will always jump to the same ring, so no need
                            ; for SS:ESP to be on the stack

after_iret:                 ; Context switch will jump to here after returning
    ret