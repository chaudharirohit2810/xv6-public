# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.
        
# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"

# Multiboot header.  Data to direct multiboot loader.
.p2align 2
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002
  #define flags 0
  .long magic
  .long flags
  .long (-magic-flags)

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
_start = V2P_WO(entry)

# Entering xv6 on boot processor, with paging off.
.globl entry
entry:
  # Turn on page size extension for 4Mbyte pages

  # <!--------
  # To tell hardware that 4MB page size is used 
  # This is done by setting 5th bit in cr4 register as 1 if that bit is 0 then 4KB page size is used
  # ----------!>
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4

  # Set page directory

  # <!---------
  # cr3 register is used to point to the page table address. This is done here
  # The entrypgdir is hardcoded value which is set in main.c (check line number 110)
  # ? what V2P_WO does? We will see that later
  # -----------!>

  movl    $(V2P_WO(entrypgdir)), %eax
  movl    %eax, %cr3

  # Turn on paging.
  
  # <!---------
  # To tell hardware that paging is turned on 
  # This is done by setting 31st bit (index starting from 1) in cr0 as 1
  # It is also made write protected by setting 17th bit as 1
  # ----------!>
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Set up the stack pointer.

  # <!--------
  # This stack will be used by main function
  # The size of stack is 4KB
  # It is added here because stack grows downwards
  # 
  # ? But where is stack variable
  # See line number 104
  # -------!>
  movl $(stack + KSTACKSIZE), %esp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.

  # <!----------
  # This will start main function in main.c
  # ---------!>
  mov $main, %eax
  jmp *%eax

# <!-----------
# .comm allocates size for the varialbe
# Here KSTACKSIZE will be allocated to stack variable
# ------------!>
.comm stack, KSTACKSIZE
