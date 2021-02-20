#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

/*
* To intialize the IDT table to handle interrupts
*/

void
tvinit(void)
{
  int i;

  /* 
    * SETGATE is used to set the idt array to point to the proper code to execute when various interrupts occur
    * Arguements to it
    * First is the idt array index
    * istrap: 1 means don't disable interrupt and 0 means disable interrupts
    * SEG_KCODE is code segment selector for interrupts 
    * vectors[i] are the different offset in interrupt code segment (present in vectors.S)
    * Last is descriptor priviledge level 0 (as kernel can only execute it) and DPL_USER (is 3) as user can execute system call
  */

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

/*
* Makes IDTR pointer (which is in hardware) point to IDT Table
* This is done for every processor
*/
void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
/* 
* This function is used to handle all the interrupts
* The arguement is pushed on to the stack in trapasm.S and vectors.S combined
*/
void
trap(struct trapframe *tf)
{
  // * If the trap is system call
  if(tf->trapno == T_SYSCALL){
    // * If the process is killed no need to execute syscall
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;    // * To store current trapframe in process
    syscall(); // * Execute the syscall
    if(myproc()->killed)
      exit();
    return;
  }

   // * To handle Controller interrupts from keyboards, timers
  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
