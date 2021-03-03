# Files run in this order:

### 1. bootasm.S

-   cli: Disable all the interrupts until bootloader loads OS
-   The below block of code makes all the segments zero
-   Seta20.1 and Seta20.2 are special lines of code
-   lgdt gdtdesc (block of code): This line will load gdt descriptors which will used to map virtual address tophysical address
-   ljmp SEG_KCODE << 3, $start32: This will be used to point %cs to Kernel code segment and %eip to start32 label
-   Then start32 will run as eip is pointing to it
-   Then data segment, stack segment and extra segment will point to SEG_KDATA << 3
-   And now as normal function call esp will point to start and bootmain (which is in bootmain.c file) is called
-   **_The below is yet to be understood _**

### 2. bootmain.c

-   first elf will point to empty space available in main memory
-   readseg((uchar\*)elf, 4096, 0):
    -   arguements are
        -   where to store from harddisk
        -   number of byts to read
        -   offset on the harddisk (this value is in bytes not in sectors)
    -   The elf is cast to uchar to read byte by byte code
    -   **_Readseg code explained_**:
        -   The basic use of it is to load kernel code from hard disk to main memory
        -   first epa is initialized to end limit of the address
        -   The function readsect at line 95 reads whole sector from the harddisk but we want to load the data at paif we do not minus the offset % SECTSIZE then whole sector will be loaded from pa but we don't want wholesector to start from pa as we need it to point particular data in segment thats why we minus the offset
        -   Then offset is converted to point to particular sector (offset is 0 for elf as kernel code is at sector 1)
        -   Now for loop runs until the pa is equal to epa (end limit) and reads each sector and stores it in mainmemory at first arguement provided
            -   **_readsect code explained_**
                -   waitdisk: Wait for harddisk to be ready
                -   Then below code is something that will copy harddisk data to main memory
                -   Then insl is called: **Still need to find out what it does**, The sector size is divided by 4 asit reads 4 bytes at a time
-   Then the if statement uses **Magic number** to check if the the file loaded is elf file or not (If it is not thenthe below block of code in bootasm.S is used to handle error (What it does? I don't know))
-   Until now elf header and program header are loaded into main memory now we need to load .text, .code and .data segments
-   Loading programs:
    -   ph will point to program header (phoff gives program header offset)
    -   eph will point to end of program headers (elf->phnum are number of program headers and when you add it phthen (ph + size of program header \* phnum is what eph points to))
    -   Now each program header will be copied to main memory using for loop
        -   pa points where the program shoud be loaded in main memory
        -   Then readseg is called to copy the hard disk data to main memory (**ph-filesz: size of program header,ph->off: offset of program in hard disk**)
        -   ph->memsz is used to make each program have equal size for uniformality.
        -   If the size is not equal to that then the if code copies 0 at remaining address
-   Now as kernel code is loaded we will start executing it: elf->entry points to entry label in entry.S and entry inbootmain.c is function pointer it, we will call it now

### 3. entry.S

-   First block of code in entry(line no 46-49) turn on page size to be 4 MB
    -   This is done by setting 5th bit in cr4 as 1 (CR4_PSE is 5th bit one and all other bit as 0)
    -   If 5th bit is 0 then page size is 4KB
-   **cr3 register points to start of page table**: This is done by line51-52 as start of entrypgdir that is thearray in main.c which stores all the page entries
-   Then particular bits are set in cr0 register to tell hardware that paging is enabled (17th bit to tell that the cr0 is write protected and 31st bit to tell that paging is enabled)
-   Then stack pointer will point to stack (initialized in main.c) + KSTACKSIZE (4096)
-   Then the below block of code will start main function in main.c
