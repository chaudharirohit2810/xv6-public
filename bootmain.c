// Boot loader.
//
// Part of the boot block, along with bootasm.S, which calls bootmain().
// bootasm.S has put the processor into protected 32-bit mode.
// bootmain() loads an ELF kernel image from the disk starting at
// sector 1 and then jumps to the kernel entry routine.

#include "types.h"
#include "elf.h"
#include "x86.h"
#include "memlayout.h"

#define SECTSIZE  512

void readseg(uchar*, uint, uint);

void
bootmain(void)
{

  //* Everyone of these variables will be on stack as we are pointing esp in bootasm.S
  struct elfhdr *elf; // ELF header structure
  struct proghdr *ph, *eph; // Program header structure
  void (*entry)(void); // Function pointer for entry into the kernel
  uchar* pa; // program address

  /*
  * elf pointer will point to 0x10000 (that is 128 KB)
  */
  elf = (struct elfhdr*)0x10000;  // scratch space

  // * This will copy 4kb from harddisk to address on physical memory pointed by elf 
  // Read 1st page off disk
  // * elf is casted to uchar to read byte by byte
  readseg((uchar*)elf, 4096, 0);

  // Is this an ELF executable?
  if(elf->magic != ELF_MAGIC) // * To check if it elf file
    return;  // let bootasm.S handle error

  // Load each program segment (ignores ph flags).
  ph = (struct proghdr*)((uchar*)elf + elf->phoff);

  // * This will give you end of program headers
  eph = ph + elf->phnum;

  // *  Iterate over each program header entry
  for(; ph < eph; ph++){

    // * Where to store program header in main memory
    pa = (uchar*)ph->paddr;

    // * Load program header from hard disk to main memory at pa 
    // * The program header has size of filesz and offset at which it is present is off
    readseg(pa, ph->filesz, ph->off);

    // * Fill the remaining bits (that are memsz - filesz) with 0s
    // * memsz = Size of program header in main memory
    // * filesz = Actual size of program header
    if(ph->memsz > ph->filesz)
      stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
  }

  /*
   * elf->entry was set by Linked using kernel.ld
   * This is address 0x80100000 specified in kernel.ld
  */

  // Call the entry point from the ELF header.
  // Does not return!
  // * Typecasting of elf->entry into function pointer which takes no arguement and returns nothing
  entry = (void(*)(void))(elf->entry);
  entry();
}

void
waitdisk(void)
{
  // Wait for disk ready.
  while((inb(0x1F7) & 0xC0) != 0x40)
    ;
}

// Read a single sector at offset into dst.
void
readsect(void *dst, uint offset)
{
  // Issue command.
  // * This is writing to disk controller to read from particular sector
  waitdisk();
  outb(0x1F2, 1);   // count = 1
  outb(0x1F3, offset);
  outb(0x1F4, offset >> 8);
  outb(0x1F5, offset >> 16);
  outb(0x1F6, (offset >> 24) | 0xE0);
  outb(0x1F7, 0x20);  // cmd 0x20 - read sectors

  // Read data.
  // * This is waiting for disk to be ready
  waitdisk();

  // * This is loop instruction to read data from disk contoller to ram
  
  /*
  ? Why the third arguement is SECTSIZE/4
  * Because insl reads 4 bytes at a time (check "l" at last) which will eventually read whole sector
  */
  insl(0x1F0, dst, SECTSIZE/4); 
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked.
void
readseg(uchar* pa, uint count, uint offset)
{
  // * This is the final address
  uchar* epa;

  epa = pa + count;

  // Round down to sector boundary.
  /*
  ? Why we minus offset?
  * Because readsect below these instruction reads the whole segment and we want to store particular value at pa therefore we will 
  * minus the value offset % SECTSIZE so address gets loaded correctly 
  */
  pa -= offset % SECTSIZE;

  // * Translate from bytes to sectors; kernel starts at sector 1.
  offset = (offset / SECTSIZE) + 1; // Kernel starts a sector 1 and bootloader is at sector 0

  // If this is too slow, we could read lots of sectors at a time.
  // We'd write more to memory than asked, but it doesn't matter --
  // we load in increasing order.
  // * Read each sector from disk 
  for(; pa < epa; pa += SECTSIZE, offset++)
    readsect(pa, offset); // Address and sectorn number are passed
}
