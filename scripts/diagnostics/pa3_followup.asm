; B24 PA3 packaging: companion routines appended to the unchanged standard ASM.
; Timing to SPLT disable reproduces V4 isr_adjust's PA3 path. The earlier arm
; removes 24 NOPs before enable and restores them before disable; the adjustment
; terminal adds exactly 1024 NOPs (16 * 64 us) before enable. No hardware timing
; claim: verify the production-T80 trace and then the original Plus photograph.
pa3_seen equ 0xBF18

            macro PA3_CASE _entry_, _isr_, _early_, _extra_, _r5_
_entry_:
            ld hl,_isr_ : ld (sync_addr),hl
            ld a,_r5_ : ld (pa3_r5),a
            jp pa3_init
_isr_:
            push af : push bc : push de : push hl
            ; Preserve the original taken branch and instruction timings.
            ld a,(test_var) : or a : jr nz,.terminal
            ld a,32 : jr .delay
.terminal:  ld a,39
.delay:     ld e,a
            ld bc,ADJ_COARSE
.wait:      dec bc : ld a,b : or c : jr nz,.wait
            ds ADJ_FINE-_early_+_extra_,0
            ld a,e : ld (ASIC_SPLT),a
            ld hl,0x6421 : ld (hl),15
            ds 4,0
            ld (hl),0
            ld a,(test_var) : or a : jp nz,.disable
.disable:
            if _early_
            ds _early_,0
            endif
            xor a : ld (ASIC_SPLT),a
            ; Original second dash starts at C16, inside active display.
            ; Move it into the right border C41-48, after display C40 and
            ; before HSYNC C49. Timing through disable is unchanged.
            ds 25,0
            ld hl,0x6421 : ld (hl),15
            ds 4,0
            ld (hl),0
            ld a,2 : ld (ASIC_SSA),a
            xor a : ld (ASIC_SSA+1),a
            ld a,1 : ld (frame_done),a
            pop hl : pop de : pop bc : pop af
            ei : ret
            endm

            PA3_CASE init_normal_late,handler_normal_late,0,0,16
            PA3_CASE init_normal_early24,handler_normal_early24,24,0,16
            PA3_CASE init_adjustment_late,handler_adjustment_late,0,1024,16
            PA3_CASE init_adjustment_early24,handler_adjustment_early24,24,1024,16
            PA3_CASE init_r5_zero,handler_r5_zero,0,0,0

pa3_r5 equ 0xBF19
pa3_init:   ld a,1 : ld (test_var),a
            xor a : ld (pa3_seen),a
            call fill_bank0
            ; Same red overlays and green SSA0200 span as standard PA3.
            ld hl,0x0790 : ld b,8
.plane:     push bc : push hl
            ld d,h : ld e,l : inc de
            ld bc,79 : ld (hl),0 : ldir
            pop hl : ld de,0x0800 : add hl,de
            pop bc : djnz .plane
            ld hl,crtc_adjust : call crtc_table
            ld a,(pa3_r5) : ld e,a : ld a,5 : call crtc_set
            ld a,2 : ld (ASIC_SSA),a
            xor a : ld (ASIC_SSA+1),a
            call overlay_sync
            ld a,255 : ld (ASIC_PRI),a
            ld bc,0x7F00 : ld a,0x9E : out (c),a
            xor a : ld (frame_done),a
            ei
.loop:      halt
            ld a,(frame_done) : or a : jr z,.loop
            xor a : ld (frame_done),a
            ld a,(pa3_seen) : or a : jr nz,.keys
            ; A latched status in display RAM, not a blanked raster pulse.
            ; It is written only after the handler returned. Outside the
            ; measured adjustment/frame0-7 spans on both physical banks.
            inc a : ld (pa3_seen),a
            ld hl,pa3_reached_text : call print_block
.keys:      call key_check
            jp .loop
