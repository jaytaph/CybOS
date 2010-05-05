[bits 32]

section .text

global task_switch                     ; Linker resolve symbol

; void task_switch (int tss_selector)
task_switch:
        push    ebp                     ; Save stack pointer
        mov     ebp, esp


        mov     eax, [ebp+8]            ; Set tss_selector
        mov     [call_far_struct+4],eax

        call far [call_far_struct]      ; Call the new task, offset is ignored, segment is a tss selector

        ; return point from the task loaded

        ; clear NT bit
        pushf                           ; Copy flags into eax
        pop     eax
        and     eax, 0xbfff             ; clear NT flag
        push    eax                     ; Set eax back into flags
        popf

        mov     esp, ebp                ; Restore stack pointer and return
        pop     ebp
        ret


section .data

call_far_struct                 ; Temp Structure for CALL FAR
        dd 0
        dd 0

