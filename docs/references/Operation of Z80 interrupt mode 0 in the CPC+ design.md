<!--
Converted from "Operation of Z80 interrupt mode 0 in the CPC+ design.html" on 2026-08-26 using pandoc (HTML → GFM).
-->

# Operation of Z80 interrupt mode 0 in the CPC+ design

The basic steps for Z80 interrupt mode 0 are:

1.  Device issues a interrupt request to the Z80
2.  If maskable interrupts are enabled:
    - the Z80 will acknowledge the interrupt
    - The interrupting device will "see" the interrupt acknowledge and put a 8-bit interrupt-vector onto the Z80 databus.
    - The interrupt-vector becomes the first opcode byte of a Z80 instruction.
    - If the instruction has more than one opcode, these opcodes are fetched.
    - the instruction is executed by the Z80.

    If maskable interrupts are disabled:
    - the Z80 will ignore the interrupt request and will not generate a interrupt acknowledge

This interrupt mode can't be used reliably on the CPC. The 8-bit interrupt-vector read by the Z80 will normally be &FF, but this is not guaranteed if expansion peripherals are connected. (When &FF is read, the equivalent of a "RST 38H" instruction is executed, and interrupt mode 0 will act the same as interrupt mode 1). Therefore, because the 8-bit interrupt-vector is not always the same it is not advisable to use this interrupt mode on the CPC.

The CPC+ hardware can generate a 8-bit interrupt-vector, and this is normally used with Z80 Interrupt mode 2. The 8-bit interrupt-vector depends on the source of the interrupt (raster interrupt or a interrupt from a DMA channel). The 8-bit interrupt vector is reliable and can be predicted, and as a result the CPC+ can use the Z80 interrupt mode 0.

However, the number of instructions that can be executed in Z80 interrupt mode 0 on the CPC+ is limited, because there is a limited set of numbers that can be defined by the CPC+ 8-bit interrupt vector.

The 8-bit interrupt vector is constructed as follows:

- Bit 2..0 is defined by the ASIC and is 0 for DMA channel 2, 2 for DMA Channel 1, 4 for DMA Channel 0 or 6 for Raster interrupt.
- Bit 7..3 is user-programmable

(See "Arnold V" Specification, Issue 1.4, section 2.7 and Extra CPC+ information for more details)

As a result a limited set of instructions can be executed.

I used the following test programs and observed the results:

;; program 1 org &8000 ;; disable interrupts di ;; unlock asic to gain access to asic registers ld b,&bc ld hl,sequence ld e,17 .seq ld a,(hl) out (c),a inc hl dec e jr nz,seq ;; page-in asic registers ld bc,&7fb8 out (c),c ;; set z80 interrupt mode 0 im 0 ;; set 8-bit interrupt vector (bits 7..3). ;; bit 0 = automatically clear interrupts ld a,0 ld (&6805),a ;; page-out asic registers ld bc,&7fa8 out (c),c ;; enable interrupts ei ;; wait for next interrupt halt ;; the halt instruction will wait for a interrupt-request. When this is received, the ;; program counter is advanced to the next instruction before the interrupt is executed. ;; So after "halt" has been executed, the program counter will be pointing ;; to the "LD D,E" instruction. ;; a "dummy" instruction ld d,e di ;; disable interrupts so we can change interrupt mode safely im 1 ;; set Z80 interrupt mode 1 (see comment for "BRK" below) brk ;; this is a special instruction provided by the Maxam assembler/dissassembler/monitor ;; when executed a dump of the register values is displayed. This instruction assembles ;; to a RST 30H. This instruction uses the Amstrad firmware to execute, and the Amstrad ;; firmware requires interrupt mode 1. ;; this is the sequence to unlock the ASIC extra features .sequence defb &ff,&00,&ff,&77,&b3,&51,&a8,&d4,&62,&39,&9c,&46,&2b,&15,&8a,&cd,&ee

In the code above, the only interrupts that will be active will be the standard raster interrupts which occur approximatly every 52 scan-lines (the other interrupt sources must be explicitly enabled and triggered for an interrupt to occur from these sources). When a raster interrupt has been signalled, the ASIC will provide a 8-bit interrupt-vector with bits 2..0 set to 6. In the program I have set the remaining bits to 0, so the complete 8-bit interrupt-vector will be &06, which is the equivalent of the "LD B,n" instruction. ("LD B,n" is a multi-byte opcode consisting of two bytes. The first byte is the opcode, and the second byte is the value of "n". After this instruction has been executed B will have the value defined by "n")

Before this program was executed B has the value &7F, but after this program had executed B had the value &53. &53 is the opcode for the "LD D,E" instruction.

I found that if I changed the "LD D,E" instruction to another instruction, the value of B would always be the same as the first opcode-byte of the replacement instruction. I also found that the instruction following the "HALT" was always executed, therefore the program counter was not incremented when the additional opcode bytes were fetched.

Therefore the second byte of the "LD B,n" instruction is always being loaded from the current program counter location.

Using program 1, I could use any opcode where bits 2..0 are "6".

To use other instructions, I would need to enable another interrupt source:

;; program 2 org &8000 ;; disable interrupts di ;; unlock asic to gain access to asic registers ld b,&bc ld hl,sequence ld e,17 .seq ld a,(hl) out (c),a inc hl dec e jr nz,seq ;; page-in asic registers ld bc,&7fb8 out (c),c ;; wait for end of vsync ;; I use this so I can predict when the next raster interrupt will occur ;; In the CPC and CPC+ design a raster interrupt is triggered on the second HSYNC after the ;; the start of the VSYNC signal. ld b,&f5 .l1 in a,(c) rra jr nc,l1 .l2 in a,(c) rra jr c,l2 ;; set interrupt mode 0 im 0 ld hl,&4030 ;; dma instruction "issue interrupt request and stop executing dma list" ld (&9000),hl ld hl,&9000 ld (&6c04),hl ;; set dma channel 1 address (source of dma instruction list for channel 1) ld a,0 ld (&6c06),a ;; set dma channel 1 prescalar ld a,&20 ld (&6805),a ;; set interrupt vector ;; clear raster counter (has the effect of forcing next raster interrupt to not ;; occur closer than 52-HSYNCs). ld bc,&7f00+%10011100 out (c),c ld a,%00000010 ;; enable dma channel 1, first instruction will execute on the next HSYNC ld (&6c0f),a ;; page-out asic registers ld bc,&7fa0 out (c),c ;; enable interrupts ei ;; wait for interrupt to occur halt ;; "dummy" instructions ld d,h ld e,l di ;; disable interrupts so we can change interrupt mode safely im 1 ;; set Z80 interrupt mode 1 (see comment for "BRK" below) brk ;; this is a special instruction provided by the Maxam assembler/dissassembler/monitor ;; when executed a dump of the register values is displayed. This instruction assembles ;; to a RST 30H. This instruction uses the Amstrad firmware to execute, and the Amstrad ;; firmware requires interrupt mode 1. ;; this is the sequence to unlock the ASIC extra features .sequence defb &ff,&00,&ff,&77,&b3,&51,&a8,&d4,&62,&39,&9c,&46,&2b,&15,&8a,&cd,&ee

This program sets up a simple dma list with a single instruction. The DMA instruction triggers a interrupt and stops the execution of the list. Before the DMA list is executed, the raster counter is reset, this forces the next raster interrupt to occur 52 lines later. As a result the DMA interrupt is serviced before the raster interrupt. DMA channel 1 is used, and bits 2..0 of the 8-bit vector will be set to &02. I have setup bits 7..3 to &20, making the final 8-bit interrupt-vector &22. &22 is the "LD (nnnn),HL" instruction. (The "LD (nnnn),HL" is a three byte opcode. The second and third byte define "nnnn" and is the memory address at which HL is written.).

After this program was executed I found that HL had been written to address &5454. &54 is the opcode for the "LD D,H" instruction. Since bits 7..0 and bits 15..8 of the address were the same, the same byte must have been read twice. This implies that the program counter was not incremented for any of the opcode byte fetches. I also found that DE matched the value of HL, therefore the "LD D,H" and "LD E,L" instructions were executed.

I found that if I changed the "LD D,H" instruction, the address was always generated from the first opcode-byte of the replacement instruction. Therefore the opcode bytes were fetched from the current program counter location.

From these experiments I found the following:

- the first byte of the opcode is provided by the 8-bit interrupt vector (this is generated by the CPC+ ASIC)
- the program counter (PC) register of the Z80 is not changed for additional opcode-byte fetches
- if a instruction is multi-byte, the remaining opcode bytes are fetched from the current PC address, and since the program counter doesn't change, these opcode bytes will all be the same value.
- If a instruction is executed, which will under normal operation could change the program-counter, when it is executed in interrupt mode 0, it will act the same, and the program counter will be changed in the same way. e.g. the function of the "JP","RET","CALL","JR" or "DJNZ" instructions does not change and program counter can be changed by these instructions.

I have not yet tested the following:

- Is the R register is incremented during interrupt mode 0?
- Is there is a difference in the instruction timing between instructions executed during interrupt mode 0 and instructions executed normally?
- Are maskable interrupts checked after the execution of the instruction during interrupt mode 0?

# Confirmation of the function of the IM x instructions

As a result of being able to use interrupt mode 0 with the CPC+, I was able to confirm the action of the following unofficial instructions:

| Opcode | Action       |
|--------|--------------|
| ED 4E  | same as IM 0 |
| ED 6E  | same as IM 0 |

The following program was used to confirm these instructions:

;; program 3 org &8000 ;; disable interrupts di ;; unlock asic to gain access to asic registers ld b,&bc ld hl,sequence ld e,17 .seq ld a,(hl) out (c),a inc hl dec e jr nz,seq ;; setup for interrupt mode 1 ld a,&c3 ld hl,im1_interrupt_handler ld (&0038),a ld (&0039),hl ;; setup for interrupt mode 2 ;; initialise bits 15..8 of interrupt vector ld a,&40 ld i,a ;; setup interrupt handler jumpblock ld ix,&4000 ld hl,im2_interrupt_handler ld b,0 .sim2 ld (ix+0),l ld (ix+1),h inc ix inc ix djnz sim2 ;; disable interrupts di ;; set interrupt mode ;; {insert instruction here} defb &ed,&4e ;; enable interrupts ei ;; wait for interrupt halt ;; (this is used by interrupt mode 0) nop di ;; disable interrupts so we can change interrupt mode safely im 1 ;; set Z80 interrupt mode 1 (see comment for "BRK" below) brk ;; this is a special instruction provided by the Maxam assembler/dissassembler/monitor ;; when executed a dump of the register values is displayed. This instruction assembles ;; to a RST 30H. This instruction uses the Amstrad firmware to execute, and the Amstrad ;; firmware requires interrupt mode 1. .im1_interrupt_handler ld b,1 ei ret .im2_interrupt_handler ld b,2 ei ret ;; this is the sequence to unlock the ASIC extra features .sequence defb &ff,&00,&ff,&77,&b3,&51,&a8,&d4,&62,&39,&9c,&46,&2b,&15,&8a,&cd,&ee

I tested the above program with the official instructions: "IM 0", "IM 1" and "IM 2".

- When interrupt mode 0 is used, B is set to "0".
- When interrupt mode 1 is used, B is set to "1".
- When interrupt mode 2 is used, B is set to "2".

Therefore from observing the value of B after the program had executed I could determine the function of the instructions.

# Notes

1.  The CPC+ does not support the full operation of Z80 interrupt mode 0. The CPC+ will only generate a single 8-bit interrupt-vector and will ignore additional opcode fetches for multi-byte instructions.

    So, the operation of multi-byte instructions may be different with hardware designs that fully support interrupt mode 0.

    Any signals that may be generated by the Z80 to fetch extra opcode bytes will be ignore. As a result, the observations I have seen may only apply to the CPC+ hardware design.

2.  As far as I know Z80 interrupt mode 0 has not been used in any programs. If it has been used in a CPC program, then it is likely to be unreliable when expansion peripherals are attached. If "FF" is read as the interrupt vector then this will have the same effect as interrupt mode 1. However, this will never work on the CPC+. When CPC+ features have not been enabled, the 8-bit interrupt vector will be "00" and not "FF" causing a "NOP" instruction to be executed. If CPC+ features have been enabled, and only standard interrupts are active, then a "LD B,n" instruction will be executed. It is not possible to program the CPC+ so that a 8-bit interrupt vector of "FF" is generated.
