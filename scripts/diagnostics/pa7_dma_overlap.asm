; Bounded PA7 DMA / compatible-raster overlap diagnostic.
; Build with pa7_dma_overlap.py; the builder generates pa7_dma_overlap.inc
; with four self-labelled pages, seventeen row labels, timing and font data.
;
; Code is copied from cartridge bank 0 at 0000 into RAM 8000-BFFF and runs
; with both ROMs off. Screen RAM is C000-FFFF; the Plus ASIC page is 4000-7FFF.
; Runtime timing code is at 2000, copied interrupt handlers at 3000-33FF,
; IM2 vectors at BE00-BEFF, result rows at B000-B10F, variables at BF00-BF7F,
; and stack at BFF0. All interruptible code and the interrupted instruction
; stream have A13=1.

ASIC_PRI       equ 0x6800
ASIC_SPLT      equ 0x6801
ASIC_SSA       equ 0x6802
ASIC_SSCR      equ 0x6804
ASIC_IVR       equ 0x6805
ASIC_DCSR      equ 0x6C0F
TIMING_BUF     equ 0x2000
TIMING_LEN     equ 3600
ISR_BASE       equ 0x3000
ISR_CATCH      equ 0x3333
IM2_TABLE      equ 0xBE00
RESULTS        equ 0xB000
RESULT_STRIDE  equ 16
RESULT_ROWS    equ 17
STACK          equ 0xBFF0
MARKER         equ 0xBF30
TRIAL_REC      equ 0xBF20

CASE_FAR_EARLY equ 0
CASE_LATE      equ 1
CASE_DMA_ONLY  equ 2
CASE_NO_DMA    equ 3
CASE_DMA_SWEEP equ 4

PAGE           equ 0xBF00
PAGE_STATUS    equ 0xBF01
CASE_INDEX     equ 0xBF02
REPETITION     equ 0xBF03
PATCH_ADDR     equ 0xBF04             ; two bytes; zero until the first patch
TRIAL_DONE     equ 0xBF06
CASE_KIND      equ 0xBF07
REC_PTR        equ 0xBF08             ; current B000 result row
SCREEN_PTR     equ 0xBF0A             ; current C000 display row fields
KEY_ARMED      equ 0xBF0C
POST_PRI       equ 0xBF0D
DMA_ENABLE     equ 0xBF0E
ISR_SOURCE     equ 0xBF31

            org 0x8000

; ---- Cold boot. The cartridge bank is copied before switching to its RAM copy.
boot:       di
            ld sp,STACK
            ld bc,0x7F00
            ld a,0xC0 : out (c),a       ; base RAM configuration
            ld a,0x8A : out (c),a       ; mode 2, upper ROM off, lower ROM on
            ld hl,0 : ld de,0x8000 : ld bc,0x4000
            ldir
            jp start

start:      ld bc,0x7F00
            ld a,0x8E : out (c),a       ; both ROMs off
            ld bc,0xF782 : out (c),c    ; V5 PPI setup: A out, B in, C out
            ld bc,0xF600 : out (c),c
            ld bc,0xBC00
            ld hl,unlock_seq
            ld e,17
.unlock:    ld a,(hl) : out (c),a : inc hl
            dec e : jr nz,.unlock
            ld bc,0x7F00
            ld a,0xB8 : out (c),a       ; ASIC register page at 4000-7FFF
            ld a,0x8E : out (c),a
            ld a,START_PAGE : ld (PAGE),a
            xor a : ld (PAGE_STATUS),a : ld (KEY_ARMED),a
            call common_init
            ld hl,handler_source : ld de,ISR_BASE : ld bc,handler_end-ISR_BASE
            ldir
            call install_im2_table
            call reset_timing_buffer
            jp run_page

; ---- V5-compatible Plus, CRTC, screen and palette setup.
common_init:
            ld bc,0x7F00
            ld a,0x8E : out (c),a
            ld a,0x70 : ld (ASIC_DCSR),a ; all DMA off; flags W1C
            xor a
            ld (ASIC_PRI),a
            ld (ASIC_SPLT),a
            ld (ASIC_SSA),a : ld (ASIC_SSA+1),a
            ld (ASIC_SSCR),a
            ld hl,0x6004 : ld de,8 : ld b,16
.sprites:   ld (hl),a : add hl,de : djnz .sprites
            ld hl,palette : ld de,0x6400 : ld bc,4
            ldir
            ld de,0x6420 : ld bc,2 : ldir                        ; dark pen 0, bright pen 1, blue border
            ld hl,crtc_std : call crtc_table
            ld hl,0xC000 : ld de,0xC001 : ld bc,0x3FFF
            ld (hl),0 : ldir
            ret

; HL -> (register,value) pairs terminated by FF.
crtc_table: ld a,(hl) : cp 0xFF : ret z
            ld bc,0xBC00 : out (c),a : inc hl
            ld a,(hl) : inc hl
            ld b,0xBD : out (c),a
            jr crtc_table

; V5 print-block format: flags, screen address, zero-terminated uppercase text;
; FF ends a block. The font and generated strings come from plus_hw_probes.py.
print_block:
            ld a,(hl) : cp 0xFF : ret z
            ld c,0 : rra : jr nc,.plain
            ld c,0xFF
.plain:     inc hl
            ld e,(hl) : inc hl : ld d,(hl) : inc hl
.char:      ld a,(hl) : inc hl
            or a : jr z,print_block
            push hl
            call glyph
            pop hl
            inc de
            jr .char

; A = character, DE = Mode 2 cell address, C = invert mask.
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

; Any keyboard key or joystick fire, released before the press.
; This scan is copied from the existing V5 hardware-probe helper.
scan_any:   ld bc,0xF40E : out (c),c
            ld bc,0xF6C0 : out (c),c
            ld bc,0xF600 : out (c),c
            ld bc,0xF792 : out (c),c
            ld e,0xFF : ld d,0x40
.line:      ld b,0xF6 : out (c),d
            ld b,0xF4 : in a,(c)
            and e : ld e,a
            inc d : ld a,d : cp 0x4A : jr nz,.line
            ld bc,0xF782 : out (c),c
            ld bc,0xF600 : out (c),c
            ld a,e
            ret

; ---- Page lifecycle.
run_page:   di
            ld sp,STACK
            xor a
            ld (PAGE_STATUS),a : ld (CASE_INDEX),a : ld (REPETITION),a
            ld (PATCH_ADDR),a : ld (PATCH_ADDR+1),a
            call reset_timing_buffer
            call clear_results
            ld hl,0xC000 : ld de,0xC001 : ld bc,0x3FFF
            ld (hl),0 : ldir
            call setup_page
            call render_progress
.case:      ld a,(CASE_INDEX)
            cp RESULT_ROWS
            jr nc,page_done
.trial:     call prepare_trial
            call arm_trial
            call commit_trial
            call render_record
            ld a,(REPETITION) : inc a : ld (REPETITION),a
            call render_progress
            ld a,(REPETITION)
            cp 8
            jr nz,.trial
            xor a : ld (REPETITION),a
            ld a,(CASE_INDEX) : inc a : ld (CASE_INDEX),a
            jr .case

page_done:  ld hl,done_text : call print_block
            ld a,0x80 : ld (PAGE_STATUS),a ; publish only after final rendering
wait_key:   call scan_any
            inc a
            jr nz,.pressed
            ld a,1 : ld (KEY_ARMED),a
            jr wait_key
.pressed:   ld a,(KEY_ARMED) : or a
            jr z,wait_key
            xor a : ld (KEY_ARMED),a
            ld a,(PAGE) : inc a : cp 4 : jr c,.page_ok
            xor a
.page_ok:   ld (PAGE),a
            jp run_page

; Reset the 3600-NOP buffer and its trailing DI/RET. The window is rebuilt at
; each page, then only the previous three-byte patch is removed per trial.
reset_timing_buffer:
            ld hl,TIMING_BUF : ld de,TIMING_BUF+1 : ld bc,TIMING_LEN-1
            ld (hl),0 : ldir
            ld a,0xF3 : ld (TIMING_BUF+TIMING_LEN),a
            ld a,0xC9 : ld (TIMING_BUF+TIMING_LEN+1),a
            ret

; Initialize all 17 fixed 16-byte records. Vector/status/mark slots are FF.
clear_results:
            ld hl,RESULTS : ld b,RESULT_ROWS
.row:       ld (hl),0 : inc hl             ; IRQ count
            ld (hl),0 : inc hl             ; pre-DCSR
            ld (hl),0 : inc hl             ; post-DCSR
            ld (hl),0 : inc hl             ; overflow/invalid flags
            ld (hl),0xFF : inc hl           ; vector/status/mark x 3
            ld (hl),0xFF : inc hl
            ld (hl),0xFF : inc hl
            ld (hl),0xFF : inc hl
            ld (hl),0xFF : inc hl
            ld (hl),0xFF : inc hl
            ld (hl),0xFF : inc hl
            ld (hl),0xFF : inc hl
            ld (hl),0xFF : inc hl
            ld (hl),0 : inc hl             ; repeat disagreements
            ld (hl),0 : inc hl             ; completed trials
            ld (hl),0 : inc hl             ; reserved
            djnz .row
            ret

setup_page:
            ; Page 2 is manual DCSR clear; all other pages use automatic clear.
            ld a,(PAGE) : cp 2 : jr nz,.auto
            ld a,1 : jr .ivr
.auto:      xor a
.ivr:       ld (ASIC_IVR),a
            ; Width 12 only on page 3; all other pages use width 8.
            ld a,(PAGE) : cp 3 : jr nz,.w8
            ld e,0x8C : jr .r3
.w8:        ld e,0x88
.r3:        ld a,3 : call crtc_set
            ld a,(PAGE) : ld l,a : ld h,0
            add hl,hl : ld de,page_text_table : add hl,de
            ld e,(hl) : inc hl : ld d,(hl) : ex de,hl
            call print_block
            ret

; A=CRTC register, E=value.
crtc_set:   ld bc,0xBC00 : out (c),a
            ld b,0xBD : out (c),e
            ret

render_progress:
            ld hl,progress_text : call print_block
            ld a,(CASE_INDEX)
            ld de,0xC73E            ; row 23, col 14: current case in hex
            call print_hex
            ld a,(REPETITION)
            ld de,0xC746            ; row 23, col 22: repetition in hex
            jp print_hex

; Prepare the trial with interrupts masked. Only the row's calibrated EI
; location and the selected one-byte X instruction vary within the buffer.
prepare_trial:
            di
            ld hl,(PATCH_ADDR)
            ld a,h : or l : jr z,.no_old_patch
            ld (hl),0 : inc hl : ld (hl),0 : inc hl : ld (hl),0
.no_old_patch:
            ld a,(CASE_INDEX) : ld l,a : ld h,0
            add hl,hl : add hl,hl : add hl,hl : add hl,hl
            ld de,RESULTS : add hl,de : ld (REC_PTR),hl
            ; CASE_INDEX*2 selects the generated calibrated delay table.
            ld a,(CASE_INDEX) : add a,a : ld l,a : ld h,0
            ld de,case_delay_table : add hl,de
            ld e,(hl) : inc hl : ld d,(hl)
            ld a,(PAGE) : cp 3 : jr nz,.delay_ready
            inc de : inc de : inc de : inc de ; W12 phase adjustment
.delay_ready:
            ld hl,TIMING_BUF : add hl,de
            ld (PATCH_ADDR),hl
            ld (hl),0xFB                    ; EI
            inc hl
            ld a,(PAGE) : cp 1 : jr nz,.nop_x
            ld (hl),0x7E                    ; X = LD A,(HL), HL points at marker
            jr .x_ready
.nop_x:     ld (hl),0x00                    ; X = NOP
.x_ready:   inc hl
            ld (hl),0x73                    ; immediately after X: LD (HL),E
            ; Clear the prior trial tuple and initialize all three slots unused.
            ld hl,TRIAL_REC : ld de,TRIAL_REC+1 : ld bc,12
            ld (hl),0 : ldir
            ld hl,TRIAL_REC+4 : ld de,TRIAL_REC+5 : ld bc,8
            ld (hl),0xFF : ldir
            xor a : ld (MARKER),a
            ; Resolve the case type, arm DMA registers while disabled, and set
            ; an early PRI=2 interrupt as the deterministic frame synchronizer.
            ld a,(CASE_INDEX) : ld l,a : ld h,0
            ld de,case_kind_table : add hl,de
            ld a,(hl) : ld (CASE_KIND),a
            ld d,0 : ld e,1
            cp CASE_DMA_ONLY : jr nz,.not_dma_only
            ld d,255
.not_dma_only:
            cp CASE_NO_DMA : jr nz,.enable_ready
            ld e,0
.enable_ready:
            ld a,d : ld (POST_PRI),a
            ld a,e : ld (DMA_ENABLE),a
            ld a,0x70 : ld (ASIC_DCSR),a
            ld hl,0x4030 : ld (0x1000),hl ; DMA0: INT | STOP, one-shot
            ld hl,0x1000 : ld (0x6C00),hl ; SAR0
            xor a : ld (0x6C02),a           ; PPR0=0
            ld bc,0x7F00 : ld a,0x9E : out (c),a
            ld a,2 : ld (ASIC_PRI),a
            ld a,0xC3 : ld (0x0038),a
            ld hl,im1_sync : ld (0x0039),hl
            im 1
            xor a : ld (TRIAL_DONE),a
            ret

; Commit BF20..BF2C against the row's first tuple. Row bytes 13 and 14 are
; the repeat disagreement counter and number of completed trials.
commit_trial:
            ld hl,(REC_PTR)
            ld a,(REPETITION)
            or a : jr nz,.compare
            ex de,hl : ld hl,TRIAL_REC : ld bc,13 : ldir
            jr .completed
.compare:   ld de,TRIAL_REC : ld b,13
.compare_byte:
            ld a,(de) : cp (hl) : jr nz,.different
            inc de : inc hl : djnz .compare_byte
            jr .completed
.different: ld hl,(REC_PTR) : ld de,13 : add hl,de
            ld a,(hl) : inc a : ld (hl),a ; disagreements, capped naturally at 7
.completed:
            ld hl,(REC_PTR)
            ld de,14 : add hl,de
            ld a,(REPETITION) : inc a : ld (hl),a
            ret

; Render the retained first observation, live repeat count, and anomaly flags.
render_record:
            ; C000 + (5+case)*80 + 16 = C1A0 + case*80.
            ld a,(CASE_INDEX) : ld l,a : ld h,0
            add hl,hl : add hl,hl : add hl,hl : add hl,hl
            ld d,h : ld e,l
            add hl,hl : add hl,hl
            add hl,de
            ld de,0xC1A0 : add hl,de
            ld (SCREEN_PTR),hl
            ld hl,(REC_PTR) : ld bc,0 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,2 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,4 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,5 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,5 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,8 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,6 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,11 : add hl,bc : ex de,hl
            call print_nibble
            ld hl,(REC_PTR) : ld bc,7 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,13 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,8 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,16 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,9 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,19 : add hl,bc : ex de,hl
            call print_nibble
            ld hl,(REC_PTR) : ld bc,10 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,21 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,11 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,24 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,12 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,27 : add hl,bc : ex de,hl
            call print_nibble
            ld hl,(REC_PTR) : ld bc,1 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,31 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,2 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,36 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,13 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,41 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,14 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,46 : add hl,bc : ex de,hl
            call print_hex
            ld hl,(REC_PTR) : ld bc,3 : add hl,bc : ld a,(hl)
            ld hl,(SCREEN_PTR) : ld bc,51 : add hl,bc : ex de,hl
            call print_hex
            ret

; A = hex byte, DE = first Mode 2 text cell.
print_hex:  push af
            rrca : rrca : rrca : rrca
            and 0x0F : call print_nibble
            inc de
            pop af
            and 0x0F
            ; fall through
print_nibble:
            and 0x0F : cp 10 : jr c,.digit
            add a,'A'-10 : jr .draw
.digit:     add a,'0'
.draw:      ld c,0
            jp glyph

; Install a 256-byte IM2 table filled with 33: every unassigned vector safely lands at
; catchall=3333. The four ASIC source bytes 00/02/04/06 get dedicated entries.
install_im2_table:
            ld hl,IM2_TABLE : ld de,IM2_TABLE+1 : ld bc,255
            ld (hl),0x33 : ldir
            ld hl,vector_words : ld de,IM2_TABLE : ld bc,8 : ldir
            ret

; Generated per-page text and timing data, plus the V5 font.
            include "pa7_dma_overlap.inc"

; ---- Copied interrupt code at 3000-33FF. Addressed while assembled at its
; eventual runtime location, then copied from handler_source to ISR_BASE.
handler_source:
            disp ISR_BASE
; Keep HALT and its return boundary on A13=1 too. The sync handler returns DI.
arm_trial:  ei : halt : di : ret
im1_sync:
            push af : push bc : push de : push hl
            ; Fixed instruction path across DMA and no-DMA controls.
            ld a,(POST_PRI) : ld (ASIC_PRI),a
            ld a,(DMA_ENABLE) : ld (ASIC_DCSR),a
            ; More than four complete scan lines under DI lets INT|STOP settle.
            ld bc,40
.wait:      dec bc : ld a,b : or c : jr nz,.wait
            ld a,(ASIC_DCSR) : ld (TRIAL_REC+1),a ; pre-DCSR
            ld hl,MARKER : ld e,1 : xor a : ld (hl),a
            ld a,0xBE : ld i,a
            im 2
            ld bc,0x7F00 : ld a,0x9E : out (c),a ; deterministic MRER reset
            scf
            call TIMING_BUF
            ; Stop PRI255 firing later while the DMA-only result is rendered.
            ld a,2 : ld (ASIC_PRI),a
            ld a,(ASIC_DCSR) : ld (TRIAL_REC+2),a ; post-DCSR
            im 1
            ld a,1 : ld (TRIAL_DONE),a
            pop hl : pop de : pop bc : pop af
            di
            ret

; IM2 DMA source vectors. Read and retain the CPU vector and DCSR status as
; independent facts. DMA vector stubs clear DMA; the raster stub never does.
isr_vec00:  push af : push bc : push de : push hl
            xor a : call isr_record
            ld a,0x70 : ld (ASIC_DCSR),a
            jp isr_exit
isr_vec02:  push af : push bc : push de : push hl
            ld a,2 : call isr_record
            ld a,0x70 : ld (ASIC_DCSR),a
            jp isr_exit
isr_vec04:  push af : push bc : push de : push hl
            ld a,4 : call isr_record
            ld a,0x70 : ld (ASIC_DCSR),a
            jp isr_exit
isr_vec06:  push af : push bc : push de : push hl
            ld a,6 : call isr_record
            jp isr_exit

; Catchall for unassigned even ASIC vectors. Odd/corrupted bus vectors are
; outside this table guarantee. Its tuple records FF and
; flag 2; unlike DMA-source stubs it does not infer cleanup from DCSR bit 7.
isr_catchall:
            push af : push bc : push de : push hl
            ld a,0xFF : call isr_record
            jp isr_exit

isr_record: ld (ISR_SOURCE),a
            ; Unexpected DMA2/DMA1/unknown sources are retained and flagged.
            cp 0 : jr z,.invalid
            cp 2 : jr z,.invalid
            cp 4 : jr z,.source_ok
            cp 6 : jr z,.source_ok
.invalid:   ld a,(TRIAL_REC+3) : or 2 : ld (TRIAL_REC+3),a
.source_ok: ld a,(TRIAL_REC)
            ld b,a : inc a : ld (TRIAL_REC),a
            ld a,b : cp 3 : jr nc,.no_slot
            ld c,a : add a,a : add a,c ; slot = BF24 + old_count * 3
            add a,0x24 : ld l,a : ld h,0xBF
            ld a,(ISR_SOURCE) : ld (hl),a : inc hl
            ld a,(ASIC_DCSR) : ld (hl),a : inc hl
            ld a,(MARKER) : ld (hl),a
            jr .guard
.no_slot:   ld a,(TRIAL_REC+3) : or 1 : ld (TRIAL_REC+3),a
.guard:     ld a,(TRIAL_REC) : cp 4
            ret

isr_exit:   ld a,(TRIAL_REC) : cp 4 : jr nc,.storm
            pop hl : pop de : pop bc : pop af
            ei : reti
.storm:     pop hl : pop de : pop bc : pop af
            di : reti

vector_words:
            dw isr_vec00,isr_vec02,isr_vec04,isr_vec06
            ; Catch target uses equal bytes so the uniform fill covers all
            ; unassigned even ASIC vector offsets without malformed addresses.
            ds ISR_CATCH-$,0
            assert $ == ISR_CATCH
            jp isr_catchall
handler_end:
            ent

; Keep physical source bytes and target placement inside the assigned regions.
            assert ISR_CATCH+3 <= 0x3400
            assert TIMING_BUF+TIMING_LEN+2 <= ISR_BASE
            assert RESULTS+RESULT_ROWS*RESULT_STRIDE <= 0xB110
            assert 0xBF00+0x80 <= STACK
            assert DELAY_BASE-1600 >= 0
            assert DELAY_BASE+24+4+3+256 <= TIMING_LEN
            assert $ <= 0xB000
