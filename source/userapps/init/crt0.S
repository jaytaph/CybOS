[bits 32]

section .text

extern main
extern exit

[global _start]
_start:
  call main
  call exit
.wait:
  hlt
  jmp .wait

