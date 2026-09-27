; Plus/GX4000 hardware probes: one cartridge, one self-labelled screen per test.
; Any key or joystick fire advances to the next screen (wrapping to the title).
; Assemble through plus_hw_probes.py, which generates plus_hw_probes.inc
; (test table, screen text, PRI phase-band table, font, START_TEST).
;
; Memory: cartridge bank 0 is copied to RAM 8000-BFFF and runs there with both
; ROMs off. Screen C000-FFFF. ASIC page at 4000-7FFF. IM1 handlers are copied
; to (or jumped from) RAM 0038. Variables live below the stack at BF00.

ASIC_PRI    equ 0x6800
ASIC_SPLT   equ 0x6801
ASIC_SSA    equ 0x6802
ASIC_SSCR   equ 0x6804
ASIC_IVR    equ 0x6805
ASIC_DCSR   equ 0x6C0F
PEN0_G      equ 0x6401          ; pen 0 green nibble; red->yellow marker
STACK       equ 0xBFF0

test_index  equ 0xBF00
frame_done  equ 0xBF01
key_armed   equ 0xBF02
band_ptr    equ 0xBF04
tmp         equ 0xBF06
irq_count   equ 0xBF07
window_r3   equ 0xBF0D          ; A: R3 during the window (0 = unchanged)
sync_addr   equ 0xBF08          ; handler the 0038 JP overlay targets
target_head equ 0xBF0A          ; first 3 bytes of the installed test handler
SYNC_LINE   equ 2               ; A/C: arming interrupt, well before line 7
COUNT_ADDR  equ 0xC000+20*80+72 ; IRQ/FRAME digits

            org 0x8000

; ---- Cold boot: executed from cartridge ROM at 0000, position independent
; until the jump into the RAM copy.
boot:       di
            ld sp,STACK
            ld bc,0x7F00
            ld a,0xC0 : out (c),a       ; base RAM configuration
            ld a,0x8A : out (c),a       ; mode 2, upper ROM off, lower ROM on
            ld hl,0 : ld de,0x8000 : ld bc,0x4000
            ldir                        ; copy the whole bank to RAM
            jp start

start:      ld bc,0x7F00
            ld a,0x8E : out (c),a       ; both ROMs off
            ld bc,0xF782 : out (c),c    ; PPI: A out, B in, C out
            ld bc,0xF600 : out (c),c
            ld bc,0xBC00                ; ASIC unlock sequence
            ld hl,unlock_seq
            ld e,17
.unl:       ld a,(hl) : out (c),a : inc hl
            dec e : jr nz,.unl
            ld bc,0x7F00
            ld a,0xB8 : out (c),a       ; RMR2: ASIC register page at 4000
            ld a,0x8E : out (c),a
            xor a : ld (key_armed),a
            ld a,START_TEST : ld (test_index),a

; ---- Run the selected test from a clean machine state.
run_test:   di
            ld sp,STACK
            im 1
            call common_init
            ld a,(test_index)
            ld l,a : ld h,0 : add hl,hl : add hl,hl
            ld de,test_table : add hl,de
            ld e,(hl) : inc hl : ld d,(hl) : inc hl
            ld a,(hl) : inc hl : ld h,(hl) : ld l,a
            push de
            call print_block
            ret                         ; into the test's init routine

common_init:
            xor a : ld (window_r3),a
            ld bc,0x7F00
            ld a,0x8E : out (c),a
            ld a,0x70 : ld (ASIC_DCSR),a    ; DMA off, DMA flags cleared
            xor a
            ld (ASIC_PRI),a
            ld (ASIC_SPLT),a
            ld (ASIC_SSA),a : ld (ASIC_SSA+1),a
            ld (ASIC_SSCR),a
            ld hl,0x6004 : ld de,8 : ld b,16
.mag:       ld (hl),a : add hl,de : djnz .mag   ; all sprites disabled
            ld a,1 : ld (ASIC_IVR),a
            ld hl,palette : ld de,0x6400 : ld bc,4
            ldir                        ; pen 0 red, pen 1 green
            ld hl,palette+4 : ld de,0x6420 : ld bc,4
            ldir                        ; border blue, sprite colour 1 white
            ld hl,crtc_std
            call crtc_table
            ld hl,0xC000 : ld de,0xC001 : ld bc,0x3FFF
            ld (hl),0 : ldir            ; screen all pen 0
            ret

; HL -> (reg, value) pairs ending with reg 0xFF.
crtc_table: ld a,(hl) : cp 0xFF : ret z
            ld bc,0xBC00 : out (c),a : inc hl
            ld a,(hl) : inc hl
            ld b,0xBD : out (c),a
            jr crtc_table

; A = register, E = value.
crtc_set:   ld bc,0xBC00 : out (c),a
            ld b,0xBD : out (c),e
            ret

; Green RA2 line on character rows 0-19 (the reference grid).
fill_grid:  ld hl,0xD000 : ld de,0xD001 : ld bc,20*80-1
            ld (hl),0xFF : ldir
            ret

; ---- Text. Block entries: flags (bit0 invert, 0xFF ends), dw address, text, 0.
print_block:
            ld a,(hl) : cp 0xFF : ret z
            ld c,0 : rra : jr nc,.plain
            ld c,0xFF
.plain:     inc hl
            ld e,(hl) : inc hl : ld d,(hl) : inc hl
.chr:       ld a,(hl) : inc hl
            or a : jr z,print_block
            push hl
            call glyph
            pop hl
            inc de
            jr .chr

; A = character, DE = RA0 byte address, C = invert mask.
glyph:      push de
            sub 32 : ld l,a : ld h,0
            add hl,hl : add hl,hl : add hl,hl
            push bc
            ld bc,font : add hl,bc
            pop bc
            ld b,8
.row:       ld a,(hl) : xor c : ld (de),a : inc hl
            ld a,d : add a,8 : ld d,a
            djnz .row
            pop de
            ret

; ---- Main loops.
; Interrupt tests: sleep in HALT. Each frame's arming (sync) handler runs the
; test window with its own NOPs, then sets frame_done; the loop then shows the
; frame's test-handler count and scans the keyboard, far from any test event.
main_irq:   halt
            ld a,(frame_done) : or a : jr z,main_irq
            xor a : ld (frame_done),a
            ld a,(irq_count) : ld b,a
            xor a : ld (irq_count),a
            ld a,b : call show_count
            call key_check
            jp main_irq

; Two decimal digits of A at COUNT_ADDR.
show_count: ld c,0
.tens:      cp 10 : jr c,.units
            sub 10 : inc c : jr .tens
.units:     push af
            ld a,c : add a,'0'
            ld de,COUNT_ADDR : ld c,0
            call glyph
            pop af : add a,'0'
            ld de,COUNT_ADDR+1 : ld c,0
            jp glyph

; Static tests: interrupts off, scan once per frame.
main_static:
            di
.frame:     ld b,0xF5
.vs_lo:     in a,(c) : rra : jr c,.vs_lo
.vs_hi:     in a,(c) : rra : jr nc,.vs_hi
            call key_check
            jr .frame

; Any key or joystick input, released then pressed, selects the next test.
key_check:  call scan_any
            inc a                       ; zero when nothing is pressed
            jr nz,.pressed
            ld a,1 : ld (key_armed),a
            ret
.pressed:   ld a,(key_armed) : or a : ret z
            di
            xor a : ld (key_armed),a
            ld a,(test_index) : inc a
            cp NTESTS : jr c,.store
            xor a
.store:     ld (test_index),a
            jp run_test

; A = AND of keyboard lines 0-9 (0xFF when nothing is pressed).
scan_any:   ld bc,0xF40E : out (c),c
            ld bc,0xF6C0 : out (c),c
            ld bc,0xF600 : out (c),c
            ld bc,0xF792 : out (c),c    ; port A input
            ld e,0xFF
            ld d,0x40
.line:      ld b,0xF6 : out (c),d
            ld b,0xF4 : in a,(c)
            and e : ld e,a
            inc d : ld a,d : cp 0x4A : jr nz,.line
            ld bc,0xF782 : out (c),c
            ld bc,0xF600 : out (c),c
            ld a,e
            ret

; Copy HL..DE (exclusive) to 0038.
install_isr:
            ex de,hl : or a : sbc hl,de
            ld b,h : ld c,l : ex de,hl
            ld de,0x0038
            ldir
            ld hl,0x0038 : ld de,target_head : ld bc,3
            ldir
            ret

; 0038 overlays: JP (sync_addr) while arming, the test handler while armed.
overlay_sync:
            ld a,0xC3 : ld (0x0038),a
            ld hl,(sync_addr) : ld (0x0039),hl
            ret
overlay_target:
            ld hl,target_head : ld de,0x0038 : ld bc,3
            ldir
            ret

; Clear pending classic/raster state and start interrupt-driven display.
; HL = sync handler, A = its PRI line.
arm_irq:    ld (sync_addr),hl
            ld (ASIC_PRI),a
            call overlay_sync
            ld hl,count_text : call print_block
            ld bc,0x7F00
            ld a,0x9E : out (c),a       ; MRER with interrupt reset
            xor a : ld (frame_done),a : ld (irq_count),a
            ei
            jp main_irq

; A/C arming interrupt on SYNC_LINE: PRI:=7, then a 480 us window (lines
; ~3-10) in which the test handler runs; nothing else executes meanwhile.
; PRI is rewritten first, before a crossing HSYNC's ordinary request on this
; same line (C0=R2+1) can match SYNC_LINE a second time. A tests switch R3 to
; the tested width only inside the window: a width that never requests would
; otherwise silence this arming interrupt as well.
isr_sync1:  push af
            ld a,7 : ld (ASIC_PRI),a
            push bc : push de : push hl
            ld a,(window_r3) : or a : jr z,.armed
            ld e,a : ld a,3 : call crtc_set
.armed:     call overlay_target
            ei
            ds 480,0
            di
            ld a,(window_r3) : or a : jr z,.rearm
            ld e,0x8B : ld a,3 : call crtc_set
.rearm:     ld a,SYNC_LINE : ld (ASIC_PRI),a
            call overlay_sync
            ld a,1 : ld (frame_done),a
            pop hl : pop de : pop bc : pop af
            ei
            ret

; ---- Title screen.
init_title: jp main_static

; ---- A: PRI width. Byte-identical handler timing to the 2026-09-26
; flat-plane probe (pri-planes-4-12.cpr): EXX, 4 NOPs, SSCR<-AC, 8 NOPs,
; SSCR<-8C. The marker is the green RA2 plane shown on line 8 (row 1 RA0).
init_width1: ld e,0x81 : jr init_width
init_width2: ld e,0x82 : jr init_width
init_width3: ld e,0x83
init_width: ld a,e : ld (window_r3),a
            call fill_grid
            ld a,0x8C : ld (ASIC_SSCR),a
            ld hl,isr_width : ld de,isr_width_end : call install_isr
            exx : ld hl,ASIC_SSCR : ld c,0xAC : exx
            ld hl,isr_sync1 : ld a,SYNC_LINE
            jp arm_irq

isr_width:  exx
            ds 4,0
            ld (hl),c
            ds 8,0
            ld (hl),0x8C
            exx
            push hl : ld hl,irq_count : inc (hl) : pop hl
            ei
            ret
isr_width_end:

; ---- Palette marker handler: pen 0 goes yellow for about 9 us.
isr_pal:    exx
            ld (hl),c
            ds 8,0
            ld (hl),b
            exx
            push hl : ld hl,irq_count : inc (hl) : pop hl
            ei
            ret
isr_pal_end:

pal_regs:   exx : ld hl,PEN0_G : ld bc,0x000F : exx
            ret

; ---- B: PRI write phase. One band per character row: an interrupt on line
; S=T-2 enters a calibrated NOP sled, writes PRI=T at a chosen C0 on line T,
; then opens a 150 us interrupt window. A request produces a yellow marker
; around the start of line T+1 (a row's RA0).
init_phase: call fill_grid
            ld hl,isr_pal : ld de,isr_pal_end : call install_isr
            call pal_regs
            ld hl,band_table : ld (band_ptr),hl
            ld hl,isr_sync : ld a,BAND_S0
            jp arm_irq

isr_sync:   push af : push bc : push de : push hl
            call overlay_target
            ld hl,(band_ptr)
            ld a,(hl) : inc hl
            ld e,(hl) : inc hl
            ld d,(hl) : inc hl
            ld (band_ptr),hl
            ld c,a : ld a,SLED_MAX : sub c
            ld c,a : ld b,0
            ld hl,phase_sled : add hl,bc
            jp (hl)
phase_sled: ds SLED_MAX,0
            ld a,e
            ld (ASIC_PRI),a             ; the phased write
            ei
            ds 150,0                    ; request window
            di
            ld a,d : or a : jr nz,.next
            ld hl,band_table : ld (band_ptr),hl
            ld a,1 : ld (frame_done),a
            ld a,BAND_S0
.next:      ld (ASIC_PRI),a
            call overlay_sync
            pop hl : pop de : pop bc : pop af
            ei
            ret

; ---- C: HSYNC crossing into the PRI line (R3 width 8, PRI=7).
init_cross49: ld e,49 : jr init_cross
init_cross57: ld e,57 : jr init_cross
init_cross58: ld e,58 : jr init_cross
init_cross62: ld e,62 : jr init_cross
init_cross63: ld e,63
init_cross: ld a,2 : call crtc_set
            ld a,3 : ld e,0x88 : call crtc_set
            call fill_grid
            ld hl,isr_pal : ld de,isr_pal_end : call install_isr
            call pal_regs
            ld hl,isr_sync1 : ld a,SYNC_LINE
            jp arm_irq

; ---- D: SPLT near the 312-line wrap. R12/R13 shows the red C000 bank; SSA=0
; selects the green 0000 bank (filled here; its labels are drawn inverted).
init_split54: ld a,54 : jr init_split
init_split55: ld a,55 : jr init_split
init_split56: ld a,56 : jr init_split
init_split57: ld a,57
init_split: ld (tmp),a
            ld hl,0x0000 : ld de,0x0001 : ld bc,0x3FFF
            ld (hl),0xFF : ldir
            ld hl,split_bank0_text : call print_block
            ld a,(tmp) : ld (ASIC_SPLT),a
            jp main_static

; ---- E: SSCR vertical offset with R9=11 (12-line rows).
init_r9_off0: xor a : jr init_r9
init_r9_off5: ld a,0x50
init_r9:    ld (ASIC_SSCR),a
            ld hl,crtc_r9 : call crtc_table
            jp main_static

; ---- F: sprite register write mirrors. Sprites 0-4, 16x16 white, all set to
; x1 magnification through +4, then &0A (x2) written to +3, +5, +6 or +7.
init_mirror:
            call sprite_pixels
            ld hl,mirror_sprites : call sprite_table
            ld a,0x0A
            ld (0x6008+3),a             ; sprite 1 offset +3 (Y high)
            ld (0x6010+5),a             ; sprite 2 offset +5
            ld (0x6018+6),a             ; sprite 3 offset +6
            ld (0x6020+7),a             ; sprite 4 offset +7
            jp main_static

; ---- G: sprite left edge, and the SSCR[7] first-character mask.
init_edge:  call sprite_pixels
            ld hl,edge_sprites : call sprite_table
            jp main_static
init_mask:  call sprite_pixels
            ld hl,mask_sprites : call sprite_table
            ld a,0x80 : ld (ASIC_SSCR),a
            jp main_static

; Sprites 0-4 solid colour 1.
sprite_pixels:
            ld hl,0x4000 : ld de,0x4001 : ld bc,0x04FF
            ld (hl),1 : ldir
            ret

; HL -> entries: sprite number, X word, Y word, magnification; 0xFF ends.
sprite_table:
            ld a,(hl) : cp 0xFF : ret z
            inc hl
            add a,a : add a,a : add a,a
            ld e,a : ld d,0x60
            ld bc,5 : ldir              ; X lo, X hi, Y lo, Y hi, magnification
            jr sprite_table

unlock_seq: db 0xFF,0x00,0xFF,0x77,0xB3,0x51,0xA8,0xD4,0x62,0x39,0x9C,0x46,0x2B,0x15,0x8A,0xCD,0xEE
; &6400 pen 0 red, pen 1 green; &6420 border blue, &6422 sprite colour 1 white.
palette:    db 0xF0,0x00, 0x00,0x0F, 0x0F,0x00, 0xFF,0x0F
crtc_std:   db 0,63, 1,40, 2,49, 3,0x8B, 4,38, 5,0, 6,25, 7,30, 8,0, 9,7, 12,0x30, 13,0, 0xFF
crtc_r9:    db 9,11, 4,25, 6,16, 7,20, 0xFF

mirror_sprites:
            db 0 : dw 64,40 : db 0x05
            db 1 : dw 160,40 : db 0x05
            db 2 : dw 256,40 : db 0x05
            db 3 : dw 352,40 : db 0x05
            db 4 : dw 448,40 : db 0x05
            db 0xFF
edge_sprites:
            db 0 : dw -64,24 : db 0x0D
            db 1 : dw -63,56 : db 0x0D
            db 2 : dw -16,88 : db 0x05
            db 3 : dw -15,120 : db 0x05
            db 4 : dw 0,152 : db 0x05
            db 0xFF
mask_sprites:
            db 0 : dw 0,24 : db 0x05
            db 1 : dw 8,56 : db 0x05
            db 2 : dw 16,88 : db 0x05
            db 0xFF

            include "plus_hw_probes.inc"

            assert $ < 0xBF00
