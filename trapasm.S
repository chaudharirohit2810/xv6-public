#include "mmu.h"

  # vectors.S sends all traps here.

  # <!-------------
  # The load will come here from vectors.S
  #
  # IMP POINT
  # If you check the trapframe structure from bottom until now ss, esp, eflags, cs, eip are pushed on to the stack
  # Then in below vector code err (which is 0) and trapno (for eg. 64 for system call) are pushed on to the stack
  #
  # ? Why they are pushed from the bottom
  # Because according to calling convention args are passed in reverse order 
  # ------------!>

.globl alltraps
alltraps:
  # Build trap frame.

  # These values are pushed according to trap frame
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushal # * This instruction will push eax, ecx, edx, ebx, oesp, ebp, esi and edi in order
  
  # Set up data segments.
  # This will set data segment (which is initialized in gdt (If you remember))
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es

  # Call trap(tf), where tf=%esp
  # * As function is called therefore current stack pointer will be pushed
  pushl %esp
  call trap # * This will call trap function in trap.c 
  addl $4, %esp

  # <!------------
  #  As this is function call therefore after trap is executed function will return here and all the registers will get the inital value
  # ------------!>

  # Return falls through to trapret...
.globl trapret
trapret:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret # This is used to return to user code from interrupt. This will setup ss, esp, eflags, cs and eip values of user process
