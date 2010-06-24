[bits 32]

section .text

extern main
extern exit

[global start]
[global _start]
start:
_start:
  call main
  call exit
.wait:
  hlt
  jmp .wait

