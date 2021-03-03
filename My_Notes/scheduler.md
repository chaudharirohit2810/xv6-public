# Steps followed on timer interrupt:

-   On timer interrupt, the code enters into vectors.S and pushes number 32 (For timer interrupt) on to the stack
-   Then code goes into the alltraps and all the registers are pushed on to the stack
-   Then code jumps into trap function in trap.c
-   Then some things are handled in switch (Yet to understand)
-   Then on line number 135-137 in trap.c a yield function is called
-   yield makes the process RUNNABLE (Ready to run) and calls the sched function (**Not scheduler**)
-   sched contains swtch function Which switches the context from old process to os scheduler (Switches stack from kernel stack of old process to stack of scheduler). This will push scheduler address, process context address and return address on to the stack.
-   Now the os scheduler will be called (**the code will jump to switchkvm as that address was pushed on to the stack while calling swtch in scheduler for the first time**) it will take first runnable process from the ptable (array of struct proc)
-   and will again call swtch which will switch the context from scheduler to new process (Swiches stack from stack of scheduler to **kernel** stack of new process). This will now push new process context, scheduler context address and return address(Which we used in above point) on to the stack.
-   **Remember: After swtch is called the code will not return to function as eip is changed in swtch.S due to ret instruction this eip will be now instruction after swtch in sched function**
-   Now as it is a function call it will return back to trap function in trap.c
-   Then to alltraps and will call trapret in trapasm.S w
-   The new registers will be loaded in trapret and iret will be called which will setup instruction pointer to new process
-   As the loop is infinite in scheduler function this instructions will continue
