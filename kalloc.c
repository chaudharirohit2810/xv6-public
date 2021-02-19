// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.

/*
* from main we saw that vstart is end of kernel code
* vend is 4MB virtual address
*/
void
kinit1(void *vstart, void *vend)
{
  // * Ignore the lock for now
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;


  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);

  // * This loop is iterating from start to end in chunks of 4KB (PGSIZE)
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)

// * Whenever other parts of code want to free the memory they will call kfree
void
kfree(char *v)
{
  struct run *r;

  /*
  * This just checks for some conditions
  * If the value of v is not alligned on page size or if value is less that end 
  * or v is greater PHYSTOP (Top of physical memory) (we can't go above the top of physical memory na)
  * If any one of them is true then panic which will stop the kernel code from running
  * 
  * When particular frame of these frame gets allocated then that will be removed from freelist and the freelist will point to next free page
  * (check above line kalloc function)
  */
  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  /*
  * This will fill in the next 4K bytes from v with some data (!!!!!!! NOT ZERO !!!!!!!!!!! because we are trying to fill it with junk) 
  TODO: Check code of memset function
  */
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);

  /*
  * Checkout below three lines
  * We can see this that it is pretty similar to append in singly linked list
  * First r is initialized to new block then its next is what is pointed by kmem.freelist 
  * And then kmem.freelist will point to again the main free list structure
  */
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;

  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.

// * Whenever other parts of kernel need memory they will call kalloc
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);

  /*
  * r is pointed to head of linked list
  * Then freelist pointer will point to next entry 
  * And r is returned which is freeframe
  */
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

