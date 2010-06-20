[bits 32]

section .text

; Export there functions
[GLOBAL handle_default_int]
[GLOBAL handle_syscall_int]

; These macro's define the proctected mode exception handlers
%macro ISR_NOERRCODE 1
      [GLOBAL handle_exception%1]

      ; Exceptions that do not push errorcode (we push a dummy manually)
      handle_exception%1:
        cli

        push byte 0
        push byte %1
        jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
      [GLOBAL handle_exception%1]

      ; Exceptions that do push error codes (no need to push extra)
      handle_exception%1:
        cli

        ; Error code already pushed by this expection
        push byte %1
        jmp isr_common_stub
%endmacro

; These macro define the IRQ handlers
%macro IRQ 1
      [GLOBAL handle_irq%1]

      handle_irq%1:
        cli

        push 0xA5A5A5A5      ; Dummy error code
        push byte %1         ; Current IRQ code
        jmp irq_common_stub
%endmacro


%macro STARTOFINTERRUPT 0
   pusha

   xor  eax, eax
   mov  ax, ds
   push eax

   mov   ax, 0x10   ; Set kernel data descriptor (SS and CS are automatically set because this interrupt is
   mov   ds, ax     ; made through a call-gate (which changes DPL and thus CS and SS, but not the others)
   mov   es, ax
   mov   fs, ax
   mov   gs, ax
%endmacro


%macro ENDOFINTERRUPT 0
   pop    eax               ; Pop data descriptor and fill others
   mov    ds, ax
   mov    es, ax
   mov    fs, ax
   mov    gs, ax

   popa                     ; Restore all saved registers

   add esp, 8               ; Cleans up the pushed error code and pushed ISR number

   sti
   iret                     ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
%endmacro




; ------------------------------------------------------------
; Handler for default interrupts
handle_default_int:
   cli

   push byte 0    ; Dummy code, just to make it more generic
   push byte 0    ; same here

   STARTOFINTERRUPT

   push  esp
   extern do_handle_default_int
   call do_handle_default_int
   add   esp, 4

   ENDOFINTERRUPT


; ------------------------------------------------------------
; Handler for default interrupts
handle_syscall_int:
   cli

   push byte 0    ; Dummy code, just to make it more generic
   push byte 0    ; same here

   STARTOFINTERRUPT

   push  esp                      ; ESP is the pointer to the registers pushed onto the stack during this function
   extern do_handle_syscall
   call  do_handle_syscall        ; call C-level handler
   add    esp, 4                  ; remove the old ESP

   mov    dword [esp+32], eax     ; We move the return-value from do_handle_syscall() into the stack. This way,
                                  ; it gets popped by popa below. This is our way of returning a code back to the
                                  ; caller.
   ENDOFINTERRUPT


; ------------------------------------------------------------
; Common stub. It's possible to place this inside the ISR_ERRCODE macro's above, but
; the code would then be duplicated which we don't really need. So instead the above
; macro's jump to this piece of code.
irq_common_stub:
   STARTOFINTERRUPT

   push esp
   extern do_handle_irq
   call do_handle_irq
   add esp, 4

   ENDOFINTERRUPT


; ------------------------------------------------------------
; Common stub. It's possible to place this inside the ISR_ERRCODE macro's above, but
; the code would then be duplicated which we don't really need. So instead the above
; macro's jump to this piece of code.
isr_common_stub:
   STARTOFINTERRUPT

   push esp
   extern do_handle_exception
   call  do_handle_exception
   add esp, 4

   ENDOFINTERRUPT







; Now, macro-create the IRQ handlers
IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

; Macro functions for pmode exceptions (note that 8, 10-14 use different handlers because of extra error codes)
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31