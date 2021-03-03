# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

# ------------Rohit----------
# This is not going to follow any calling convention as it involves loading the new eip
# the registers in old will be stored 
# the registers in new should be loaded on stack
# Here, esp = return addressi (which is X), esp + 4 = old context address, esp + 8 = new context address
# 
# Before moving understand that there are three types of stack
# user stack: Stack of user process
# kernel stack for process: stack for kernel processes that run on process's behalf
# kernel stack for kernel: This is for kernel code

.globl swtch
swtch:
  movl 4(%esp), %eax		#Rohit: eax = old context address
  movl 8(%esp), %edx		#Rohit: edx = new context 

  # Save old callee-saved registers
  # This is pushed on kernel stack
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
	# ----------Rohit-------------
	# Pointer old (eax) is now pointing to the current address of a stack
	# This eventually means *eax = *(old) = current stack
  movl %esp, (%eax)

	# -----------Rohit-----------
	# Now stack pointer will point to context of new process (This eventually means pointing to kernel stack of new process)
  movl %edx, %esp

  # Load new callee-saved registers

	# ---------Rohit----------------
	# CPU registers will get loaded by values given by new context 
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp

	# ----------Rohit-------------
	# The fifth value on stack is eip so it will get loaded in eip register of cpu, after this the new process will start running
  ret  
