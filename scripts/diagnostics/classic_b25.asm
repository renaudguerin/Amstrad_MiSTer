; Classic CPC instruction acceptance diagnostic, B25 V1.
; Technical information sourced from the "Amstrad CPC CRTC Compendium"
; by Longshot (CC BY-NC-ND).
; French v1.11 §27.7.2 pp289-290: HALT/NOP synchronization, phase-sensitive
; acceptance, and CRTC1 unit variance. §26 pp281-282: 1/2/3-us instructions.
; Counts are observations; no emulator-derived hardware pass/fail values.
        org 0x1000
STATUS  equ 0x8000
RESULTS equ 0x8100                 ; 9 groups x 8 trials x (PC,HL) little-endian
SLED    equ 0x4000
        di
        ld sp,0xA000
        im 1
        ld bc,0x7F8E : out (c),c  ; mode 2, both ROMs disabled
        ld bc,0x7FC0 : out (c),c  ; base 64K on 6128
        ld bc,0x7F00 : out (c),c
        ld a,0x54 : out (c),a     ; pen 0 black
        inc c : out (c),c
        ld a,0x4B : out (c),a     ; pen 1 white
        ld c,0x10 : out (c),c
        ld a,0x54 : out (c),a     ; border black (not a timing marker)
restart:
        xor a : ld (STATUS),a
        ld hl,crtc_normal : call crtc_table
        call clear_screen
        ld hl,busy_text : call print_block
        ld a,7 : ld e,0xFF : call crtc_set
        ; Let pre-existing VSYNC and the GA resynchronizer drain (>20 ms).
        ; No firmware, snapshot phase or PPI VSYNC dependency.
        ld hl,6000
.settle: dec hl : ld a,h : or l : jr nz,.settle
        ld hl,RESULTS : ld (result_ptr),hl
        ld ix,cases
        ld a,9 : ld (groups_left),a
.group:
        ld a,(ix+0)
        ld hl,SLED : ld de,SLED+1 : ld bc,4095
        ld (hl),a : ldir
        xor a : ld (SLED-1),a : ld (SLED-2),a
        ld l,(ix+1) : ld h,(ix+2) : ld (launch+1),hl
        ld a,8 : ld (trials_left),a
.trial:
        call trial
        ld hl,(result_ptr)
        ld de,(pc_capture)
        ld (hl),e : inc hl : ld (hl),d : inc hl
        ld de,(hl_capture)
        ld (hl),e : inc hl : ld (hl),d : inc hl
        ld (result_ptr),hl
        ld a,(trials_left) : dec a : ld (trials_left),a : jr nz,.trial
        inc ix : inc ix : inc ix
        ld a,(groups_left) : dec a : ld (groups_left),a : jr nz,.group
        ld hl,crtc_normal : call crtc_table
        call clear_screen
        ld hl,table_text : call print_block
        ld ix,RESULTS
        ld de,0xC000+3*80+16
        ld a,9 : ld (groups_left),a
.draw_group:
        push de
        ld b,8
.draw_pc:
        push bc
        ld l,(ix+0) : ld h,(ix+1)
        ld bc,SLED : or a : sbc hl,bc
        call hexword
        inc de
        ld bc,4 : add ix,bc
        pop bc : djnz .draw_pc
        ld bc,-32 : add ix,bc
        pop de
        ld hl,80 : add hl,de : ex de,hl
        push de
        ld b,8
.draw_hl:
        push bc
        ld l,(ix+2) : ld h,(ix+3) : call hexword
        inc de
        ld bc,4 : add ix,bc
        pop bc : djnz .draw_hl
        pop de
        ld hl,80 : add hl,de : ex de,hl
        ld a,(groups_left) : dec a : ld (groups_left),a : jr nz,.draw_group
        ld a,0xA5 : ld (STATUS),a ; published only after rendering and restoration
.release:
        call scan_any : cp 0xFF : jr nz,.release
.press: call scan_any : cp 0xFF : jr z,.press
        jp restart

; A CALL return remains under the two discarded interrupt PCs. Both requests
; come from the real GA; the first wakes HALT and anchors the common launch.
trial:
        ld a,0xC3 : ld (0x0038),a
        ld hl,arm_isr : ld (0x0039),hl
        ld bc,0x7F9E : out (c),c  ; reset interrupt counter, mode2/ROMs off
        ei
        halt
        jp $                       ; first ISR never returns here
arm_isr:
        pop af                     ; discard HALT resume PC
        ld hl,capture_isr : ld (0x0039),hl
        ld hl,0
        ld de,1
        scf                        ; RET NC must remain untaken
        ei
launch: jp SLED                    ; identical cost, only padding address changes
capture_isr:
        ld (hl_capture),hl         ; capture before any HL use
        pop hl
        ld (pc_capture),hl
        ret                        ; original CALL trial return, IFF stays clear

crtc_table:
        ld a,(hl) : cp 0xFF : ret z
        ld bc,0xBC00 : out (c),a : inc hl
        ld a,(hl) : inc hl : ld b,0xBD : out (c),a
        jr crtc_table
crtc_set:
        ld bc,0xBC00 : out (c),a : ld b,0xBD : out (c),e : ret
clear_screen:
        ld hl,0xC000 : ld de,0xC001 : ld bc,0x3FFF
        ld (hl),0 : ldir : ret

; Text: dw RA0 address, zero-terminated string. Zero address ends block.
print_block:
        ld e,(hl) : inc hl : ld d,(hl) : inc hl
        ld a,d : or e : ret z
.char:  ld a,(hl) : inc hl : or a : jr z,print_block
        push hl : call glyph : pop hl : inc de : jr .char
glyph:
        push hl : push bc : push de
        sub 32 : ld l,a : ld h,0
        add hl,hl : add hl,hl : add hl,hl
        ld bc,font : add hl,bc : ld b,8
.row:   ld a,(hl) : ld (de),a : inc hl
        ld a,d : add a,8 : ld d,a : djnz .row
        pop de : pop bc : pop hl : ret
hexword:
        ld a,h : call hexbyte : ld a,l
hexbyte:
        push af : rrca : rrca : rrca : rrca : call nibble : pop af
nibble: and 15 : add a,'0' : cp '9'+1 : jr c,.digit : add a,7
.digit: call glyph : inc de : ret

; Keyboard/joystick: no ROM calls; release then any press reruns the whole batch.
scan_any:
        ld bc,0xF40E : out (c),c
        ld bc,0xF6C0 : out (c),c
        ld bc,0xF600 : out (c),c
        ld bc,0xF792 : out (c),c
        ld e,0xFF : ld d,0x40
.line:  ld b,0xF6 : out (c),d
        ld b,0xF4 : in a,(c) : and e : ld e,a
        inc d : ld a,d : cp 0x4A : jr nz,.line
        ld bc,0xF782 : out (c),c
        ld bc,0xF600 : out (c),c
        ld a,e : ret

; A 64-us line for the measurement, R2=46/R3=8E. Normal CPC firmware
; 40-column timing restored before showing the 80-character mode2 table.
crtc_normal: db 0,63,1,40,2,46,3,0x8E,4,38,5,0,6,25,7,30,8,0,9,7,12,0x30,13,0,0xFF
cases:  db 0x00 : dw SLED           ; NOP control
        db 0x76 : dw SLED           ; HALT control
        db 0xD0 : dw SLED           ; RET NC pad0
        db 0xD0 : dw SLED-1         ; RET NC pad1
        db 0x23 : dw SLED           ; INC HL pad0
        db 0x23 : dw SLED-1         ; INC HL pad1
        db 0x19 : dw SLED           ; ADD HL,DE pad0
        db 0x19 : dw SLED-1         ; ADD HL,DE pad1
        db 0x19 : dw SLED-2         ; ADD HL,DE pad2
result_ptr: dw 0
pc_capture: dw 0
hl_capture: dw 0
groups_left: db 0
trials_left: db 0
        include "classic_b25.inc"
        assert $ < SLED-2
