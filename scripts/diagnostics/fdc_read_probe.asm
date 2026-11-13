; Standalone 6128 Plus CPR: public FDC I/O only, no firmware or test hooks.
; Cold boot copies bank 0 to 8000-BFFF, then disables both ROMs.
; Screen C000-FFFF, transfer buffer 2000-21FF, stack BFF0. ASIC page 4000
; is used only for palette initialization. Interrupts remain disabled.
; Every hardware wait is bounded. Only the final completed screen HALTs.
; The optional hold monitor deliberately does not read the data register:
; its live MSR + counter and sticky LEFT F0 flag prevent a stale ACTIVE claim.

BUFFER      equ 0x2000
; Canonical CPC MSR/data ports. The P10 simulation uses Plus aliases FBDE/FBDF.
MSR         equ 0xFB7E
SCREEN      equ 0xC000
            org 0x8000
boot:       di
            ld sp,0xBFF0
            ld bc,0x7F00
            ld a,0xC0 : out (c),a
            ld a,0x8A : out (c),a
            ld hl,0 : ld de,0x8000 : ld bc,0x4000
            ldir
            jp start
start:      ld bc,0x7F00
            ld a,0x8E : out (c),a
            ld bc,0xF782 : out (c),c
            ld bc,0xF600 : out (c),c
            ld bc,0xBC00 : ld hl,unlock_seq : ld e,17
.unlock:    ld a,(hl) : out (c),a : inc hl
            dec e : jr nz,.unlock
            ld bc,0x7F00 : ld a,0xB8 : out (c),a
            ld a,0x8E : out (c),a
            ld hl,palette : ld de,0x6400 : ld bc,4 : ldir
            ld hl,palette+4 : ld de,0x6420 : ld bc,2 : ldir
            ld hl,crtc_std
.crtc:      ld a,(hl) : cp 0xFF : jr z,.clear
            ld bc,0xBC00 : out (c),a : inc hl
            ld a,(hl) : inc hl : ld b,0xBD : out (c),a
            jr .crtc
.clear:     ld hl,SCREEN : ld de,SCREEN+1 : ld bc,0x3FFF
            ld (hl),0 : ldir
            ld hl,BUFFER : ld de,BUFFER+1 : ld bc,511
            ld (hl),0 : ldir
            ld hl,title : ld de,SCREEN+2 : call print_string
            ld hl,details : ld de,SCREEN+80+2 : call print_string
            ld hl,stage_legend : ld de,SCREEN+3*80+2 : call print_string
            ld hl,stage_text : ld de,SCREEN+4*80+2 : call print_string
            ld hl,error_text : ld de,SCREEN+5*80+2 : call print_string
            ld hl,error_legend : ld de,SCREEN+6*80+2 : call print_string
            ld hl,msr_text : ld de,SCREEN+7*80+2 : call print_string
            ld hl,count_text : ld de,SCREEN+8*80+2 : call print_string
            ld hl,mismatch_text : ld de,SCREEN+9*80+2 : call print_string
            ld hl,first_text : ld de,SCREEN+10*80+2 : call print_string
            ld hl,recal_text : ld de,SCREEN+12*80+2 : call print_string
            ld hl,raw_title : ld de,SCREEN+14*80+2 : call print_string
            ld hl,raw_empty : ld de,SCREEN+15*80+2 : call print_string
            ld hl,raw_note : ld de,SCREEN+17*80+2 : call print_string
            ld hl,reset_note : ld de,SCREEN+23*80+2 : call print_string
            ld a,1 : call set_stage
            ld bc,0xFA7E : ld a,1 : out (c),a  ; physical drive A motor
            call spinup
            ld a,2 : call set_stage
            ld hl,specify_command : ld e,3 : call send_command
            jp c,report
            ld hl,recal_command : ld e,2 : call send_command
            jp c,report
            call spinup             ; bounded time for physical seek to track 0
            ld a,8 : call write_byte
            jp c,report
            call result_byte
            jp c,report
            ld (recal_st0),a
            cp 0x80 : jp z,bad_recal ; invalid/no interrupt has only one byte
            call result_byte
            jp c,report
            ld (recal_pcn),a
            or a : jp nz,bad_recal
            ld a,(recal_st0) : and 0xE3 : cp 0x20
            jp nz,bad_recal
            ld a,3 : call set_stage
            ld hl,read_command : ld e,9 : call send_command
            jp c,report
            ld bc,MSR
            ld ix,0                 ; total non-ready poll budget = 65536
            IFDEF HOLD_ACTIVE
.wait_f0:   in a,(c)
            ld (last_msr),a
            and 0xF0 : cp 0xF0 : jp z,hold_monitor
            cp 0xD0 : jr z,early_result
            dec ix : ld a,ixh : or ixl : jr nz,.wait_f0
            jp timeout
            ELSE
            ld hl,BUFFER : ld de,0
; CPC timings, ACCC v1.11 FR section26 p281, section4.4.2 p24:
; IN=4us, JPcc=3us, CPn=2us, INC/DEC r=1us, JRcc=3/2us, INI=5us.
; Ready block is20us. Non-ready IN-to-IN interval is11us,14us on E wrap.
; From sampled-ready MSR to data I/O is<=14us, hence worst RQM phase<=28us.
; Unrolled512 blocks avoid the former per-byte count/branch overhead. INI
; reads via FB7F before decrementing B; INC B restores FB for the next MSR.
; DE is a shared65536-miss budget, touched only on non-ready paths. No screen
; work, comparison, per-byte calls or buffer-index arithmetic during transfer.
read_transfer_begin:
            include 'fdc_read_transfer.inc'
read_transfer_end:
            ld (last_msr),a
            call save_count
            jp get_results
read_timeout:
            ld (last_msr),a
            call save_count
            jp timeout
read_early:
            ld (last_msr),a
            call save_count
            ENDIF
early_result:
            ld a,2 : call record_error
get_results:
            ld a,4 : call set_stage
            ld hl,result_data : ld e,7
.result:    call result_byte
            jp c,report
            ld (hl),a : inc hl
            ld a,(result_count) : inc a : ld (result_count),a
            dec e : jr nz,.result
            jr report
bad_recal:  ld a,3 : call record_error : jr report
timeout:    ld a,1 : call record_error
report:     ld a,5 : call set_stage
            call compare_all
            ld de,SCREEN+8*80+16 : ld hl,(read_count) : call print_word
            ld de,SCREEN+9*80+16 : ld hl,(mismatch_count) : call print_word
            ld de,SCREEN+10*80+16 : ld hl,(first_offset) : call print_word
            ld de,SCREEN+5*80+14 : ld a,(error_stage) : call print_hex
            ld de,SCREEN+5*80+22 : ld a,(error_code) : call print_hex
            ld de,SCREEN+7*80+16 : ld a,(last_msr) : call print_hex
            ld de,SCREEN+12*80+18 : ld a,(recal_st0) : call print_hex
            inc de : ld a,(recal_pcn) : call print_hex
            ld hl,result_data : ld a,(result_count) : ld b,a
            ld de,SCREEN+15*80+2
            or a : jr z,.score
.raw:       ld a,(hl) : inc hl : call print_hex : inc de
            djnz .raw
.score:     ld a,(error_code) : or a : jr nz,.fail
            ld hl,(mismatch_count) : ld a,h : or l : jr nz,.fail
            ld a,(result_count) : cp 7 : jr nz,.fail
            ld hl,pass_text : jr .status
.fail:      ld hl,fail_text
.status:    ld de,SCREEN+20*80+2 : call print_string
            ld a,6 : call set_stage
stop:       di : halt : jr stop

; Inclusive all-512 comparison: unread bytes count as mismatches regardless
; of initialized RAM contents. First offset FFFF means no mismatches.
compare_all:
            ld ix,(read_count) : ld iy,0
            ld hl,BUFFER : ld de,expected_data : ld bc,512
.loop:      ld a,ixh : or ixl : jr z,.bad
            dec ix : ld a,(de) : cp (hl) : jr z,.next
.bad:       push hl
            ld hl,(mismatch_count) : inc hl : ld (mismatch_count),hl
            ld a,h : or a : jr nz,.have_first
            ld a,l : cp 1 : jr nz,.have_first
            ld (first_offset),iy
.have_first:pop hl
.next:      inc hl : inc de : inc iy
            dec bc : ld a,b : or c : jr nz,.loop
            ret
save_count: push hl : ld de,BUFFER : or a : sbc hl,de
            ld (read_count),hl : pop hl : ret

; Send E bytes at HL. A timeout records the current stage and returns carry.
send_command:
.loop:      ld a,(hl) : inc hl : call write_byte : ret c
            dec e : jr nz,.loop
            ret
write_byte: push bc : push de : push af
            ld bc,MSR : ld de,0
.poll:      in a,(c) : ld (last_msr),a
            and 0xC0 : cp 0x80 : jr z,.ready
            dec de : ld a,d : or e : jr nz,.poll
            pop af : pop de : pop bc
            ld a,1 : call record_error : scf : ret
.ready:     pop af : inc c : out (c),a
            pop de : pop bc : or a : ret
result_byte:
            push bc : push de
            ld bc,MSR : ld de,0
.poll:      in a,(c) : ld (last_msr),a
            and 0xF0 : cp 0xD0 : jr z,.ready
            dec de : ld a,d : or e : jr nz,.poll
            pop de : pop bc
            ld a,1 : call record_error : scf : ret
.ready:     inc c : in a,(c)
            pop de : pop bc : or a : ret
record_error:
            push af : ld a,(error_code) : or a : jr nz,.already
            pop af : ld (error_code),a
            ld a,(stage) : ld (error_stage),a : ret
.already:   pop af : ret

; Fixed passive delay; no VSYNC dependency or unbounded motor/seek wait.
spinup:     ld b,3
.outer:     ld hl,0
.inner:     dec hl : ld a,h : or l : jr nz,.inner
            djnz .outer
            ret

            IFDEF HOLD_ACTIVE
hold_monitor:
            ld hl,hold_text : ld de,SCREEN+20*80+2 : call print_string
            ld hl,live_text : ld de,SCREEN+8*80+2 : call print_string
            ld hl,sample_text : ld de,SCREEN+9*80+2 : call print_string
            ld hl,left_text : ld de,SCREEN+10*80+2 : call print_string
            ld iy,0
.loop:      ld bc,MSR : in a,(c) : ld (last_msr),a
            and 0xF0 : cp 0xF0 : jr z,.display
            ld a,1 : ld (left_f0),a
.display:   ld a,(last_msr) : ld de,SCREEN+8*80+16 : call print_hex
            inc iy : push iy : pop hl
            ld de,SCREEN+9*80+16 : call print_word
            ld a,(left_f0) : ld de,SCREEN+10*80+16 : call print_hex
            ld a,iyh : cp 0xFF : jr nz,.loop
            ld a,iyl : cp 0xFF : jr nz,.loop
            ld hl,monitor_done : ld de,SCREEN+20*80+2 : call print_string
            jp stop
            ENDIF

set_stage:  ld (stage),a : ld de,SCREEN+4*80+16 : jp print_hex
print_string:
.loop:      ld a,(hl) : inc hl : or a : ret z
            call glyph : inc de : jr .loop
print_word: ld a,h : call print_hex : ld a,l
print_hex:  push af
            rrca : rrca : rrca : rrca : and 15 : call nibble
            pop af : and 15
nibble:     add a,'0' : cp '9'+1 : jr c,.draw
            add a,7
.draw:      call glyph : inc de : ret
; Same Mode2 font-address pattern as plus_hw_probes.asm. Preserve callers.
glyph:      push hl : push de : push bc
            sub 32 : ld l,a : ld h,0
            add hl,hl : add hl,hl : add hl,hl
            ld bc,font : add hl,bc : ld b,8
.row:       ld a,(hl) : ld (de),a : inc hl
            ld a,d : add a,8 : ld d,a : djnz .row
            pop bc : pop de : pop hl : ret

stage:          db 0
error_stage:    db 0
error_code:     db 0
last_msr:       db 0
left_f0:        db 0
read_count:     dw 0
mismatch_count: dw 0
first_offset:   dw 0xFFFF
result_count:   db 0
recal_st0:      db 0xFF
recal_pcn:      db 0xFF
result_data:    ds 7,0
specify_command:db 3,0xDF,3
recal_command:  db 7,0
read_command:   db 0x46,0,0,0,0x41,2,0x41,0x1E,0xFF
unlock_seq: db 0xFF,0,0xFF,0x77,0xB3,0x51,0xA8,0xD4,0x62,0x39,0x9C,0x46,0x2B,0x15,0x8A,0xCD,0xEE
palette:    db 0,0,0xFF,0x0F,0x0F,0
crtc_std:   db 0,63,1,40,2,49,3,0x8B,4,38,5,0,6,25,7,30,8,0,9,7,12,0x30,13,0,0xFF
            IFDEF HOLD_ACTIVE
title:      db 'FDC READ RESET PROBE - HOLD MONITOR',0
            ELSE
title:      db 'FDC READ RESET PROBE - NORMAL',0
            ENDIF
details:    db 'DRIVE A TRACK 00 HEAD 00 R41 N02 - 512 BYTES',0
stage_legend: db '01 MOTOR 02 INIT 03 READ 04 RESULT 05 CHECK 06 DONE',0
stage_text: db 'CURRENT STAGE 00',0
error_text: db 'ERROR STAGE 00 CODE 00',0
error_legend: db 'CODE 01 TIMEOUT 02 SHORT READ 03 RECAL FAILURE',0
msr_text:   db 'LAST MSR      00',0
count_text: db 'BYTES READ    0000',0
mismatch_text: db 'MISMATCHES    0000',0
first_text: db 'FIRST OFFSET  FFFF',0
recal_text: db 'RECAL ST0 PCN   FF FF',0
raw_title:  db 'ST0 ST1 ST2 C  H  R  N',0
raw_empty:  db '-- -- -- -- -- -- --',0
raw_note:   db 'RAW RESULT STATUS AND CHRN ARE NOT SCORED',0
reset_note: db 'RESET AND LOAD NORMAL CPR FOR A FRESH READ',0
pass_text:  db 'PASS PAYLOAD AND SEVEN RAW RESULTS',0
fail_text:  db 'FAIL - SEE COUNTS AND ERROR STAGE',0
            IFDEF HOLD_ACTIVE
hold_text:  db 'LIVE HOLD - NO DATA READ - NOT A READ PASS',0
live_text:  db 'LIVE MSR      00  ',0
sample_text:db 'SAMPLE COUNT  0000',0
left_text:  db 'LEFT F0 EVER  00  ',0
monitor_done: db 'MONITOR STOPPED - LAST MSR ONLY - RESET   ',0
            ENDIF
            include 'fdc_read_probe.inc'
expected_data:
            incbin 'fdc-read-probe-expected.bin'
expected_end:
            assert expected_end-expected_data == 512
            assert $ < 0xBE00
