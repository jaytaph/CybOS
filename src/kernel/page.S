[bits 32]

section .text


[GLOBAL context_switch]
context_switch:
	pushad      ; Push all regs onto the current stack

	mov   eax, [esp+36]
	mov   [eax], esp        ; save esp into old_esp

	mov   eax, [esp+40]     ; Set new_esp to esp
	mov   esp, [eax]

	popad                   ; restore all 8 general purpose regs from the new stack

	ret                     ; return to caller (usually the bottom part of switch_task(),
	                        ; but when a new task starts, it will be it's start address



[GLOBAL read_eip]
read_eip:
  pop eax   ; Fetch and remove the return address from the stack
  jmp eax   ; Jump back to the return address. Since it's in eax,
            ; it automatically is seen as the return value.


[GLOBAL copy_physical_pageframe_data]

;
; Copies the physical data out of a (4KB) frame. Since we only know the
; physical address and we do not have a mapping to the virtual address (TODO: does the mm not know this??)
; we need to copy with paging disabled.
;
; void copy_physical_pageframe_data (void *src, void *dst);
copy_physical_pageframe_data:
  push ebx
  pushf

  mov esi, [esp+12]       ; Source (physical address)
  mov edi, [esp+16]       ; Destination (physical address)


  ; We are a bit in a pickle. This function NEEDS paging disabled, but we are on a 0xC0000000
  ; kernel address. As soon as we disable paging, the CPU does not recognize the next command
  ; (because it's on 0xC0000000). To make sure this function works, we can use 3 things:
  ;
  ; 1. Make sure the function gets called from the physical adres.
  ;    We must ensure that virtual 1:1 mapping is available to function. In that case it does
  ;    not matter wether paging is on or off. However, calling that function from C is not
  ;    easy.
  ; 2. Copy the complete function to a known location (in lower memory) and run it. Again,
  ;    this is not very easy.
  ; 3. Call the function, but make sure the disable-paging stuff is done from the physical
  ;    address. This looks like method 1, only we jump to the "known" physical address only when
  ;    we need to. Since we know the kernel runs at 0xC0000000 and the kernel physical is the
  ;    same, only minus 0xC0000000, we actually "jump" to the lower physical part.
  ;    2 conditions:
  ;       1. The kernel must have a linear mapping of 1:1. So virtual 0x1000 is physical 0x1000
  ;       2. We must know the offset of the kernel (in this case 0xC0000000)
  ;

  mov eax, virtual_address1       ; Jump to the "lower" virtual address. From there we can
  sub eax, 0xC0000000             ; disable paging without problems.
  jmp eax
virtual_address1:

  mov eax, cr0            ; Disable paging
  and eax, 0x7FFFFFFF
  mov cr0, eax

  mov ecx, 1024           ; Copy 1024 dwords (4096 bytes = 1 page)
  rep movsd

  jmp virtual_address2    ; Jump back to 0xC000xxxx and enable paging again.
virtual_address2:


  mov eax, cr0            ; Enable paging
  or  eax, 0x80000000
  mov cr0, eax

  popf
  pop ebx
  ret

