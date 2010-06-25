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

.sysexit:
  mov  eax, 15
  int  0x42

.hang:
  jmp  .hang
  





