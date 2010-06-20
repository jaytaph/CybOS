[bits 32]

section .text


; void do_context_switch (regs_t **prev_context, regs_t *next_context);
[GLOBAL do_context_switch]
do_context_switch:
    mov   eax, [esp+4]     ; 56 = regs_t previous_task->context
    mov   [eax], esp        ; save esp into previous task

    mov   eax, [esp+8]     ; Set esp of new task
    mov   esp, eax          ; 60 = regs_t previous_task->context

    ; Stack is now switched


    ; This is strange, but explainable. On a fork(), the kernel stack is copied from the parent.
    ; On the first switch to this new task we need to return directly from the iret. This is
    ; because the child task does not have all the stack-info needed to return from
    ; do_context_switch() back to switch_task() back to reschedule() etc etc, since this data was
    ; not available oin the parent stack at the time of copy (during the sys_fork).
    ;
    ; What we do here, is check if there is a valid return address on the stack (everything above
    ; 0xFFFF is considered valid). Everything below, might look like a segment-descriptor (which
    ; should always be 0x33, be i don't like to hardcode this). If we have a return-address, we
    ; assume that this process already has switched once, so all information about the right way
    ; (through context_switch() back to switch_task() etc), so we can safely return back to there
    ; and let the interrupt-handler take care of all info.
    ;
    ; When it's NOT a valid return-address, we assume this is the first switch to this task. We
    ; only know the context of the stack that is saved on interrupt. We just act like we are at
    ; the end of the interrupt. This code is and MUST be EXACTLY the same as the code that handles
    ; the normal end of interrupt. So basically we skip all the switch/reschedule stuff.
    ;
    ; The main problem is that it doesn't look like this is the 'right-way'(c) to do stuff, since
    ; i've never seen this before. However, it looks like this works and it's something I can
    ; actually understand WHY it works. Therefor, let's see how long we can use this method.
    ;
    mov   eax, [esp]
    cmp   eax, 0x0000FFFF
    jbe    .do_iret_on_first_switch

    ret


.do_iret_on_first_switch:
    ; THIS CODE COMES FROM THE MACRO  ENDOFINTERRUPT from isr.S and MUST BE THE SAME

    pop    eax               ; Pop data descriptor and fill others
    mov    ds, ax
    mov    es, ax
    mov    fs, ax
    mov    gs, ax

    popa                     ; Restore all saved registers

    add esp, 8               ; Cleans up the pushed error code and pushed ISR number

    sti
    iret                     ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
