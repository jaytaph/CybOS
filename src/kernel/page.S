[bits 32]

section .text

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

  mov eax, cr0            ; Disable paging
  and eax, 0x7FFFFFFF
  mov cr0, eax

  mov ecx, 1024           ; Copy 1024 dwords (4096 bytes = 1 page)
  rep movsd

  mov eax, cr0            ; Enable paging
  or  eax, 0x80000000
  mov cr0, eax

  popf
  pop ebx
  ret

