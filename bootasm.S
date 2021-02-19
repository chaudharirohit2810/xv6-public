#include "asm.h"
#include "memlayout.h"
#include "mmu.h"

# Start the first CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.code16                       # Assemble for 16-bit mode
.globl start                  # This is pretty similar to global that we use in c
start:
 

  cli                         # BIOS enabled interrupts; disable

  # Zero data segment registers DS, ES, and SS.
  xorw    %ax,%ax             # Set %ax to zero
  movw    %ax,%ds             # -> Data Segment
  movw    %ax,%es             # -> Extra Segment
  movw    %ax,%ss             # -> Stack Segment

   # <!------------ Here as all segments are zero therefore logical address = physical address -------------!>

  # Physical address line A20 is tied to zero so that the first PCs 
  # with 2 MB would run software that assumed 1 MB.  Undo that.
seta20.1:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.1

  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60
  outb    %al,$0x60

  # ---------------------------------Line no 21 to this is used to enable 21 bit address not that important----------------------------------------------

  # Switch from real to protected mode.  Use a bootstrap GDT that makes
  # virtual addresses map directly to physical addresses so that the
  # effective memory map doesn't change during the transition.

  # <!---- This will load gdt table and gdtdesc will point that location ----!>
  lgdt    gdtdesc         # See at the bottom of code, global descriptor table is loaded here

  # --------------------Below three instructions tells cpu that we have switched from real to protected mode-----------------------
  movl    %cr0, %eax
  orl     $CR0_PE, %eax
  movl    %eax, %cr0


//PAGEBREAK!
  # Complete the transition to 32-bit protected mode by using a long jmp
  # to reload %cs and %eip.  The segment descriptors are set up with no
  # translation, so that the mapping is still the identity mapping.


  # <!--- 
  # The first arg will be cs register and second will be ip 
  # The address is calculated as base + offset (logical address) 
  # Here as base is of code segment is zero in gdt therefore logical address = physical address 
  # 
  # IMP POINT
  # Why the SEG_KCODE is left shifted by 3?
  # The value of SEG_KCODE is 1 so SEG_KCODE << 3 will be 8
  # In 32-bit protected mode each gdt entry will be 64 bits (8 bytes) and below in the gdt table we have seen that code segment 
  # is 2nd entry in gdt so it will be at address 8 from start, this is the reason it is shifted 

  # Due to ip setting the code will now jump to start 32 entry
  # ----------!>
  ljmp    $(SEG_KCODE<<3), $start32


.code32  # Tell assembler to generate 32-bit code now.
start32:
  # Set up the protected-mode data segment registers

  # <!--- 
  # According to gdt second entry is data segment with base as 0 and limit as 4GB so 
  # SEG_KDATA (which is 2) << 3 will be 16 and it will point to second entry in gdt.
  # 
  # Data Segment, Stack segment and extra segment will point to this entry
  # FS and GS are zero because they are not used
  # --------!>

  movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  movw    $0, %ax                 # Zero segments not ready for use
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS


  # <!------ 
  # In these instructions esp will point $start i.e. 7c00 
  # 
  # IMP Observation: 
  # Why the esp is set to 7c00?
  # Because stack grows downwards while the code written is growing upwards. There is no overlap between stack and code
  #
  # Think about this: did we use stack before this? 
  #
  # As call is used therefore return address wiil be pushed on to the stack (that wiil be one instruction below call)
  # Now we will jump to bootmain in bootmain.c
  # -------!> 

  # Set up the stack pointer and call into C.
  movl    $start, %esp
  call    bootmain

  # If bootmain returns (it shouldn't), trigger a Bochs
  # breakpoint if running under Bochs, then loop.
  movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
  movw    %ax, %dx
  outw    %ax, %dx
  movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
  outw    %ax, %dx
spin:
  jmp     spin

# Bootstrap GDT
.p2align 2                                # force 4 byte alignment
gdt:
  # <!------------The first arguement is permission, second is base and third is limit------!>

  # <!------------This is the first segment also known as zero segment---------!>
  SEG_NULLASM                             # null seg 

  # <!-------------This is the second segment which is code segment it has execute and read permissions with 
  # base of 0 and limit as 4GB (Max in 32-bit) ------!>
  SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg

  # <!------------The data will be stored here, it has write permission ------!>
  SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg

gdtdesc:
  .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
  .long   gdt                             # address gdt

