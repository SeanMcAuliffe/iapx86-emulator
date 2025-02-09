# iapx86-emulator

Partial emulator for some Intel 8086 processor functions.

## Intel 8086 Registers

The 8086 has 8 general purpose 16-bit registers. Four (A through D) of them can
be accessed as if they were pairs of 8-bit registers via aliases (suffixed H for
high-byte and L for low-byte).  

Addressable as 16 or 8 bit registers:  

Primary accumulator:        AX (AH, AL)  
Base accumulator:           BX (BH, BL)  
Counter accumulator:        CX (BH, BL)  
Extended accumulator:       DX (DH, DL)  

Addressable as 16 bit registers only:   

Source index:               SI  
Destination index:          DI  
Base Pointer:               DP  
Stack Pointer:              SP   

Instruction Pointer:        IP   

Code segment:               CS  
Data segment:               DS  
Extra segment:              ES  
Stack segment:              SS   

Status register flags:      - - - - O D I T S Z - A - P - C  

## Flags

The Flag Register is a 16-bit register, 9 of the bits are used as condition code
indicators. These are the: carry flag (C), parity flag (P), auxiliary carry flag
(A), zero flag (Z), sign flag (S), trap flag (T), interrupt flag (I), direction
flag (D), and the overflow flag (O).


## Netwide Assembler

Input programs can be assembled using [NASM](www.nasm.us). Example programs are
found in the `asm` directory. The test cases for this program come from Casey
Muratori's Computer Enhance series, and can be found at
[github.com/cmuratori/computer_enhance](https://github.com/cmuratori/computer_enhance),
or at the submodule path `computer_enhance/perfaware/part1/`.


