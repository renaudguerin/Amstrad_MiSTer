# CSL/SSM implementation plan

Written 2026-09-12 for backlog item B4. Stream: **general** (shared tooling plus one
passive RTL observer). This plan follows the research in
[mister-hardware-loop-plan.md](mister-hardware-loop-plan.md) and the working B2 harness
recorded in [b2-device-capture-2026-09-12.md](b2-device-capture-2026-09-12.md). It is an
implementation brief for a fresh session; each phase is separately mergeable and has its
own gate.

**Design reviewed twice 2026-09-12, source `8731452`; rough convergence reached.**
Retain the host/RTL split. Phases 0/1 and the default-off Phase 2 prototype are in source.
Final source review is CLEAR and local acceptance gates pass; device acceptance remains
open. Phase 2 uses a single stream of rotating sample windows with the history and
publication contracts below. Fable's second verdict was “CHANGES REQUIRED,
converged. No architecture blocker.” The contracts below incorporate the accepted
findings and parent corrections; they authorize a local, default-off experimental
implementation, not bandwidth or device acceptance. See the
[review and parent disposition](csl-ssm-design-review-2026-09-12.md).

The [marker inventory](shaker-ssm-marker-inventory-2026-09-12.md) establishes that
SHAKER patches ordinary per-test codes into an `ED 00 ED 00` template at run time.
Every non-reserved SSM code requests a screenshot; `#FFFE` selects the CSL name.
The reference table has 712 code rows, 483 applicable to CRTC 0 and 478 to CRTC 1.
Those are reference coverage targets, not observed emission or capture counts.

The material review findings are:

- Freezing both pass buffers at the first marker cannot preserve later pixels for a
  second marker. Freezing at pass end can work for a fixed progressive traversal;
  general interlace or repeated address visits require more retained history.
- The current event stamp is taken **before** `amstrad_video_output`; its 8-bit
  native-dot `hpos` wraps every 256 dots. It cannot be reused as an exact image address.
- Converted RGB is 24-bit. A 12-bit capture silently loses the classic DAC conversion;
  the proposed memory and bandwidth figures therefore do not describe that tap.
- DDR3 publication, session freshness, host servicing and capture retention need
  explicit contracts. Ring magic alone proves neither a safe memory allocation nor
  that an event belongs to the current core load.

Opus produced the initial implementation after convergence; Gemini continued the
repairs and Sol independently reviewed the code. Final source verification is recorded
in the review document. Hardware acceptance and integration/build publication remain
separate gates.

## Goal

Run the Logon System SHAKER test walks from the published CSL scripts on the real MiSTer,
and label every capture with the SSM code that SHAKER itself emits, so captures map
one-to-one onto the SHAKERLAND hardware photographs and AMSpiriT reference images.
B2 today covers one hand-built case (module B, test 9). The target is every SHAKER
module and test that our core can run, driven by the author's scripts, without
per-test authoring.

**Reference coverage verified 2026-09-12.** The portal's table contains 712 distinct
codes ranging from `0001` to `040C` (with gaps), and its
`SHAKER_SCREENSHOT_CODE.xlsx` maps each to its reference image, test id, subset and
CRTC applicability. Five rows lack a test id; do not invent their module association.
Runtime traversal still has to establish which rows each disc/script visits.
Captures are named `MISTER_<crtc>_<HHLL>.png` per the standard's
suggested rule. The codes are emitted from a run-time-patched template rather than
inlined, so they are invisible to a static scan of the discs; see
[the inventory](shaker-ssm-marker-inventory-2026-09-12.md).

Non-goals: a CSL interpreter inside the FPGA, CRTC types 2/3/4, tape and snapshot
commands, and full CSL conformance claims. Unsupported commands are rejected with a
reason; nothing is silently approximated.

## The two standards in one paragraph each

**CSL v1.4** (`docs/references/Shaker_CSL/CSL-STANDARD-EN.pdf`, bundle at
`https://shaker.logonsystem.eu/Shaker_CSL.zip`) is a host-side script: one command per
line, `;` comments. Commands used by the SHAKER scripts: `csl_version`, `crtc_select`,
`reset`, `wait <µs>`, `disk_insert`, `key_delay <press> <between> [<after CR>]`,
`key_output '<text>'` with `\(RET)`-style special keys and `{...}` chords, `csl_load`.
Also defined: `wait_vsyncoffon`, `wait_driveonoff`, `wait_ssm0000`, `screenshot_name`,
`screenshot [vsync]`, `snapshot*`, `keyboard_write`, `cpc_model`, `gate_array`,
`memory_exp`, `rom_*`, `tape_*`. `wait` is in *emulated* microseconds. On error the
runner must report script name, line number, instruction, reason, script version and
supported version, and keep a log of the last run.

**SSM v1.1** (`docs/references/Shaker_CSL/SSM-STANDARD-EN.pdf`): the Z80 executes two
consecutive undefined-ED instructions `ED LL ED HH`; the code is `HH*256+LL`, recognised
after the HH byte is fetched. Any non-ED byte between the pairs resets recognition
(`ED 3F 00 ED 3E ED 3D` yields `3D3E`). LL and HH are each within the allowed ranges
`00-3F`, `7F-9F`, `A4-A7`, `AC-AF`, `B4-B7`, `BC-BF`, `C0-FD`. Reserved: `0000`
releases `wait_ssm0000`; `FFFE` (`ED FE ED FF`) = named screenshot; `FFFF` = snapshot;
`FFFD`/`FFFC` = Sikoview logging; other `FFxx` reserved. Suggested image name:
`<Emulator>_<CRTC>_<HHLL>.<ext>`. Undefined ED opcodes are two NOPs on real hardware,
so SHAKER runs identically on hardware, emulator and our core. SHAKER 2.5+ emits the
markers; an Excel sheet on the portal maps codes to SHAKERLAND images.

**Byte-set distinction:** the listed user-code ranges enumerate **175** values.
The matcher accepts the wider **177-value NOP set**, including `FE`/`FF` needed by
the reserved examples. The 31,329 combinations describe that wider set; the arithmetic
is not independent verification of the narrower user ranges. Document the permissive
matcher without extending the advertised user-code ranges. None of the 712 reference
codes uses `FE`/`FF`, so no author clarification is a prerequisite for SHAKER work.

## Why the FPGA changes the picture

The core continues running while the host sleeps, polls, injects keys or retrieves a
PNG. At normal speed a host sleep approximates CSL microseconds, but all transport and
capture time also advances the machine. The scripts' padded waits and the author's
stable marker placement make a bounded trial sensible; they do not bound how long a
screen stays unchanged after its marker. Measure service latency and state dwell time.
The SSM observer identifies an instant; only retained pixels can preserve that instant.

CSL belongs on the host: keyboard, media, reset and configuration are Main/HPS jobs.
SSM belongs in RTL: a four-byte matcher on the M1 fetch stream, passive and cheap.

## Phase 0: CSL runner on the host (no RTL)

New `scripts/hardware-loop/csl_runner.py`, reusing `SSHTransport`, MGL generation,
hash pinning and named-screenshot retrieval from [driver.py](../scripts/hardware-loop/driver.py).
The JSON case format stays for ad-hoc cases; CSL becomes the normal way to drive a
SHAKER walk.

Command semantics on this target:

| Command | Implementation | Notes |
|---|---|---|
| `csl_version` | record; warn if unknown | bundled scripts say 1.0 while using v1.4 forms |
| `reset` / `reset hard` | fresh core load through MGL | equals power-on |
| `reset soft` | reject | no distinct soft reset path from the host |
| `crtc_select 0` | CFG status bit 2 = 1 | `crtc_type = ~status[2]` in `Amstrad.sv` |
| `crtc_select 1`, `1A`, `1B` | CFG status bit 2 = 0 | 1A/1B are one type here; record which the script asked for |
| `crtc_select 2/3/4` | reject | not implemented in the core |
| `crtc_select` after the first key | accept as no-op if the effective bit is unchanged, else reject | a live change needs the OSD |
| `cpc_model 0/1/2` | CFG status bits 5:4 before load | Plus models: reject until Plus CSL is in scope |
| `disk_insert [A] 'x.dsk'` | MGL `S0` mount; drive B `S1` | path resolved under `disk_dir` or the case's media directory |
| `key_delay` | drives `MBC_KEY_WAIT` | one knob is both CSL delays; see below |
| `key_output` | CPC characters and `\(XX)` names translated to Linux keycodes for the active ROM layout, sent through MBC `raw_seq` | chords `{..}` become MBC hold/release; `\(KOF)` cannot be honoured and is logged as an approximation |
| `key_from_file` | reject | inline with `key_output` |
| `keyboard_write` | reject | no matrix injection port in the core |
| `wait` | host sleep, value in µs | logged as approximate |
| `wait_vsyncoffon`, `wait_driveonoff` | reject in Phase 0 | need core observability |
| `wait_ssm0000` | reject in Phase 0, implemented in Phase 1 | |
| `screenshot_name` | set a name for an explicit screenshot or `FFFE` | ordinary SSM codes retain code-derived names; current runner needs correction below |
| `screenshot_dir`, `disk_dir` | reject | use `--out-dir` and `--disk-dir` |
| `screenshot` | Main `screenshot <name>.png`, wait for a complete decode | `vsync` option rejected: Main captures asynchronously |
| `snapshot*`, `tape_*`, `rom_*`, `gate_array`, `memory_exp` | reject | |
| `csl_load` | recursive run, depth-limited, cycle-checked | |

CFG editing follows the B2 method: back up the 16-byte `config/Amstrad.CFG`, change only
the bits the script names, verify by hash, restore at the end. Configuration is applied,
not merely declared, but the native PNG still omits the OSD, so the manifest records
"applied by CFG" and never "visually confirmed".

Keyboard: the CSL layer is character-level; the mapping from character to Linux keycode
depends on the ROM's layout. Start from the AZERTY table that B2 established for the
French ROM and add a UK table. Keep the table in one module with a corpus test that
every character used by the 25 bundled scripts is mappable in both layouts.

MBC timing, **resolved**: the pinned `mbc.c` sleeps `inter_key_wait` before every
press *and* every release, and `MBC_KEY_WAIT` sets it. One knob is therefore
simultaneously the CSL key-press delay and the CSL inter-key delay, so
`key_delay 70000 70000` matches those two requested durations. This does not make
the entire key sequence exact: chord event ordering and invocation overhead remain.
The third parameter, the delay after a carriage return, splits the MBC invocation
and becomes a host sleep. `MBC_SEQUENCE_WAIT` is not a per-key delay: it is slept
once at the start and once at the end of each invocation for uinput settling, which
is a fixed 2 s of overhead per `key_output` and is recorded, not compensated for.
`\(KOF)` cannot be honoured, because MBC always sleeps before each event.

Runner outputs: per-run manifest (effective settings, hashes, ordered command log with
host timestamps, approximations, rejections with the six error fields the standard
lists, screenshot names and SHA-256), the retained Main log, and a `last-run.log`.

Corpus: SHAKER 2.6 (`shaker26.dsk`) with the bundled `SHAKE26*` scripts. The bundle has
no 2.7 scripts. Establish the 2.6 baseline first, then test whether those scripts also
drive 2.7; record disc/script hashes and observed coverage separately.

Tests (host, `python3 -m unittest discover -s scripts/hardware-loop`): parser over all
25 bundled scripts with no rejection other than the documented ones; keycode coverage;
chord and `\(KOF)` translation; rejection reporting fields; `csl_load` cycle detection;
dry-run producing MGL and manifest without network. Deterministic and offline.

Acceptance gate: run `MODULE_B/SHAKE26B-1.CSL` on the device from the top through test 9
(module menu, tests 1, Z, 1, 9), stopping at a script line chosen for the run. Every
screenshot is complete and repeatable across two runs. The manifest lists each
approximation and rejection. This replaces the single B2 case as the standard capture
path. **Still open: no device run has happened.**

A correction to this section: the bundled scripts contain no `screenshot` instruction
at all, so "named by the script" is not achievable in phase 0. Captures there come
from `--screenshot-at [SCRIPT:]LINE`, which asks for one after a chosen script line
without editing the author's files. Labelled captures genuinely depend on phase 1.

## Phase 1: SSM detector, event ring and OSD toggle (RTL)

**Shipped, with the differences noted inline.** The detector, event record, ring
and DDR3 writer all live in one module, `rtl/ssm_marker.v`, rather than being split
between the motherboard and `Amstrad.sv`: that is what makes the whole path
simulatable from one Verilator fixture. The motherboard exports the raw tap only.

**Detector.** New `rtl/ssm_marker.v`, header citing the SSM standard v1.1 and the
SHAKER portal (not CRTC behaviour, so no ACCC attribution is required, but keep the
Logon System credit). Input is the M1 opcode-fetch byte stream. The motherboard already
observes that byte for Plus open-bus behaviour in
[Amstrad_motherboard.v](../rtl/Amstrad_motherboard.v)
(`~M1_n & ~MREQ_n & ~RD_n`, byte from `cpu_data_bus`). Reuse the condition and prove
the SSM sample edge against CPU bus validity and wait states. One sample per
fetch: derive a fetch strobe from the falling edge of that condition, not a level.
As shipped, the motherboard exports the level and the bus byte as
`ssm_m1_fetch`/`ssm_bus_data`, and `ssm_marker` owns the edge and the byte latch, so
the logic the vectors exercise is the logic that runs.

State machine over consecutive M1 bytes: `ED` then allowed `LL` then `ED` then allowed
`HH` produces `event_stb`/`hit_code = {HH, LL}`. A non-ED byte where the second prefix
is expected resets the matcher, as in the spec's `ED 3F 00 ED 3E ED 3D` example.
A defined instruction after an ED prefix (for example `ED 4B`, `LD BC,(nn)`) resets it.
Two points matter when reviewing the implementation:
`#ED` is itself inside `C0-FD`, so `ED ED` is a complete undefined-ED instruction
carrying `LL = ED` rather than a restart, and `#FE`/`#FF` sit outside every range the
standard offers to user code yet are exactly the bytes its own reserved codes use
(`ED FE ED FF`, `ED FF ED FF`). The matcher accepts them in either byte; see the
user/NOP byte-set distinction above. Undefined ED opcodes execute in T80 as two M1
fetches like hardware (`Prefix` path in `rtl/T80/T80.vhd`), so both bytes of each pair
appear on the fetch stream. In the existing interrupt vector, intervening non-ED
handler fetches break the marker; the interrupt acknowledge itself is not an MREQ read.

**Event record, format 1 as implemented.** Word A contains `code[15:0]`, a 24-bit
VSYNC-edge count, `line[9:0]`, `hpos[7:0]` and `field`. Word B contains a 16-bit
event sequence and a 32-bit `clk_sys` tick. These counters restart on reset or disable.
The `hs`/`vs` inputs are the motherboard-selected timing tuple **before**
`amstrad_video_output`, sampled at `ce_16`. The native colour converter registers this
same stream one stage later; it is the same timebase with a stage offset, not a final
mixer/scaler coordinate.

`hpos` advances in native dots, so 256 dots wrap it in 16 us; an ordinary 64 us line
contains about 1024 dots. `line` also wraps at 1024 sync edges. Treat format 1 as coarse
event telemetry, not an unambiguous stitch address. The 32-bit tick wraps every
67.108864 s at 64 MHz. Use modular differences only within a verified session and an
interval shorter than one wrap; reset must never be interpreted as elapsed time.

**Transport to the host.** The implementation now drives the formerly idle DDRAM
write port. A 16-byte header and 64 16-byte records occupy 1040 bytes starting at
parameter `DDR_BASE = 0x30000000`. `DDRAM_ADDR` carries 64-bit word addresses; follow
the actual core route through `sys/sys_top.v` and `sys/sysmem.sv`, not just the separate
palette-reader example. Verify the allocation against the target Linux memory map and
framework/Main use **before enabling writes**. The host reads with BusyBox
`dd if=/dev/mem` over the existing SSH transport. No Main patch or device daemon is
needed for event telemetry. `status_set` and `info_req` are not event transports.

The header carries a 32-bit `written` count and an 8-bit saturating `dropped` count.
The host derives reader overrun separately. The writer has one pending event; its
suitability for bursts and DDR stalls needs measurement. Header-after-record ordering
does not make an entire host `dd` atomic when a slot is reused.

**Controlled startup, implemented 2026-09-12.** The RTL now publishes the
magic and a `written = 0` header when `enable` rises, before any marker. The host rejects a nonzero initial header and a header read
that finishes after its deadline. This is a bounded startup boundary, not a session identity: an old empty header still looks fresh. The host
pairs it with a controlled start, observing the zero state before it sends program
input, and the reader treats a count that went backwards as a restart.

**OSD toggle.** `P2O[37],SSM markers,Off,On;` now owns bit 37. Off, the default,
holds the detector in reset and suppresses writes. On, the observer has no logical
feedback into CPU, CRTC or video. This is a source-level passivity claim; DDR allocation,
timing closure and device visibility still need verification. A stalled request is
held through acceptance even after a one-clock disable, then startup is reinitialized.

**Runner integration.** `scripts/hardware-loop/ssm_ring.py` and `--ssm` read the ring.
Every non-reserved code and `FFFE` requests Main's asynchronous capture. Default names
are `MISTER_<crtc>_<HHLL>.png`, with occurrence suffixes on reuse. `wait_ssm0000` waits
on observed sync events. `--ssm` sets and restores the saved CFG bit; restoring a file
does not itself prove the running core has reapplied that bit. Exact VSYNC and drive
waits need new observations; the existing marker ring does not implement them.
The host timing, naming and sync-wait gaps below remain open.

**Simulation vectors**, shipped as `make -C sim ssm-marker-test` (19 vectors, in the
default gate). The fixture `sim/ssm_marker_top.v` runs one production `ssm_marker`
behind two drivers: a real Z80 executing bytes from RAM through the production GA
divider and WAIT, and a synthetic opcode-fetch driver. Every expected code is derived
on paper from the SSM document and cited at its assertion.

- Executing: `ED FE ED FF` (expect `FFFE`), `ED 00 ED 00` (expect `0000`), the spec
  example `ED 3F 00 ED 3E ED 3D` (expect exactly one event, `3D3E`), `ED 4B nn nn`
  (no event), and a marker reaching the ring.
- Interrupt split: `IM 1` with `EI` so the interrupt is accepted between the two
  pairs; expect no event, and the same program with `DI`; expect one event, `FFFE`.
  The acknowledge cycle asserts IORQ with MREQ high and so is not an opcode fetch;
  what drops the marker is the handler's own fetch at the vector.
- Wait states: the synthetic driver stretches each fetch to 1, 2, 5 and 13 clocks.
  One marker and one strobe per fetch in every case. This is the property the T80pa
  leg was meant to prove, and it is tested directly at the module boundary instead,
  because GHDL is not part of the default gate: `crtc-t80-test` is a separate target
  this repository's CI does not run.
- Allowed-range boundaries: every range endpoint the standard lists plus the first
  excluded value above each; and `ED ED ED ED`, which the standard's own ranges make
  a valid `EDED`.
- Ring: header fields, wrap behaviour, an Avalon `waitrequest` stall that must delay
  and never drop, and a forced drop that must still be counted.
- Toggle: enable low, expect zero events and zero DDR3 writes, and a half marker
  spanning the toggle must not complete.

### Repairs and acceptance before a full device walk

Separate the bounded diagnostic device run from full subset conformance. Its immediate
prerequisites are a verified DDR interval, fresh startup publication, useful read-error
reporting and a resolved Avalon reset/disable contract. The remaining items below are
follow-ups, not a reason to postpone a script that does not exercise them.

**Status 2026-09-12:** items 1 to 5 are implemented in the uncommitted source,
including focused host and stalled-write regressions. Final gate/review results belong
in [the review record](csl-ssm-design-review-2026-09-12.md); the DDR interval and device
startup behavior remain separate prerequisites.

1. **Publish startup before events: first-run prerequisite.** Write a magic/version
   header with `written=0` on enable, rather than waiting for the first marker. For the
   controlled BASIC/CSL load, observe that initialized state before sending program
   input, and retain the script's boot wait. Bound failures diagnostically. A host
   `consumed=0` reset alone replays old DDR records. A zero header is a useful minimal
   fix, not a general session identity: an old empty header can look fresh, or immediate
   program events can advance the count before the host sees zero. General reload or
   immediate-emission support needs an explicit startup boundary. `load_core` itself
   is asynchronous; do not infer readiness merely from sending its request.
2. **Report read failures: first-run prerequisite.** Propagate `dd` failures through
   `dd | base64 | tr`. Distinguish an empty/failed read, truncated data and invalid
   headers from bounded startup-without-magic. Today's broad `SsmRingError` catch in
   `DeviceBackend.poll_ssm` can turn them all into “no marker.”

   **Read coherence and loss: bounded follow-up.** For the sparse SHAKER diagnostic,
   use conservative header re-read/headroom checks and report overrun; do not require
   a new ring ABI first. A reader already near overrun can lose a needed slot to one
   new event, and a slot write precedes its header commit, so account for that margin.
   General concurrent-read guarantees require stronger publication/version checking.
   A drop on the header-publication cycle can leave the last published count behind
   the register; 255 means saturated, not exactly 255 losses. Keep both visible, and
   do not call a lossy run complete. Review/fix disable aborting `ddram_we` while
   `waitrequest` is asserted before relying on the observer's device safety; a protocol
   violation is not proven harmless by the absence of CPU feedback.
3. **Service events across host operations: start synchronously.** Drain at command
   boundaries and run completion, and record request/retrieval latency. The runner
   polls during sliced sleeps, but MBC key injection and Main PNG capture block it.
   Measure those gaps on the bounded walk before introducing an ingestion worker.
   Main screenshots remain serial and approximate. Use monotonic wall deadlines for
   CSL waits and `--max-wait`; summing sleep slices excludes SSH/capture time.
4. **Define the sync-wait boundary: conformance follow-up.** `wait_ssm0000` currently
   waits beyond `ssm_sync_seen`, which reflects polling, not opcode time: an older unpolled marker
   may release it, while an already-polled marker is discarded. CSL v1.4 says to wait
   until the sequence is executed without defining whether an earlier buffered event
   may satisfy the wait. Select one-shot consumption: consume one unconsumed `0000`
   event from the current controlled run, identified by ring sequence/order, including
   one received before wait entry. Log that distinction. Fresh-after-entry is another
   possible interpretation, not the selected contract or a source-established rule.
   Neither interpretation makes host polling an exact core wait.
5. **Correct capture metadata and naming: bounded follow-up.** `_ssm_capture` currently
   consumes a pending `screenshot_name` even for an ordinary code; only `FFFE` may use
   it on the SSM path.
   Its proximity heuristic marks only the later capture and still says `FFFE` in the
   warning. Mark both affected captures and treat absence of the warning as no guarantee:
   proximity does not measure phase dwell or host latency. Every Main capture remains
   approximate. The event stamp alone cannot
   establish the image's age, nor a universal lower bound of one VSYNC.

Use a few cross-boundary tests: old DDR surviving reload; slot reuse between record
word reads; a marker during a blocking key/capture operation and at script end; a
pending CSL name followed by an ordinary code then `FFFE`; and sync arrival around
wait entry. These expose interactions not covered by a parser corpus or a synthetic
held-fetch test.

Exhaustive long-run VSYNC-counter wrap coverage and unused-command conformance are not
prerequisites for the bounded walk. The default corpus does not set `screenshot_name`
or call `wait_ssm0000`; preserve their limitations until their own fixes land.

**Device order, after the minimal fixes and required implementation review:**

1. Trace and reserve the ring's byte interval against framework/Main allocations and
   the target kernel memory map with the observer disabled. `--ssm-base` changes the
   reader address only; it cannot relocate the compiled FPGA writer. Keep their bases
   identical. Do not search for a writable base by trial and error.
2. Obtain an authorized exact-source RBF through the normal integration workflow, or
   an explicitly requested pre-merge build under `ci-testing-policy.md`. A device test
   needs synthesis, but technically need not wait for a merge. This plan authorizes
   neither publication nor a new build by itself.
3. Verify a known marker and **fresh** header/count across two loads; compare detector
   off/on boot and navigation against B2. Retain RBF/media/script hashes, configuration,
   transport timings, writer drops, reader loss and startup transitions.
4. Run the bounded `MODULE_B/SHAKE26B-1.CSL` path through test 9 twice. Join observed
   ordinary codes to the reference table and retain unknown/inapplicable codes as
   diagnostics. Applicability is a cross-check, not proof of the applied machine.
5. Exercise a flashing test and record both codes, their modular tick delta, phase
   dwell time, event-to-request latency, PNG retrieval time and time until the next
   burst. One ordinary 50 Hz period is about 1.28 M ticks, but SHAKER can change or
   remove VSYNC; measure tick intervals rather than assuming 50 Hz.
6. Later, exercise `wait_ssm0000` with a small purpose-written CSL/program pair,
   including a missing-marker timeout. This closes that feature's separate gate;
   the bundled scripts do not need it.

Recorded implementation gates at `8731452`: `make -C sim`, lint and host tests were
reported passing (19 SSM vectors; 108 host tests). This review reran only the focused
ring-reader suite (29 tests passed), not simulation or the full host gate. Run the
required simulation/host gates for repairs and obtain fresh
cross-provider implementation review before integration; this review does not retire
[the two implementation review-debt rows](review-debt.md).

## Phase 2: exact frame capture (separate gate)

**Consumer confirmed; simplify before implementation.** The first candidate is one
native write stream into bounded time windows, with host decoding and finite retention.
It removes mirrored writes, per-address cloning and a mandatory host-release mailbox.
Fable's suggestion motivates this direction; its pass stamps and sizing are not accepted
without the counterexamples in [the review disposition](csl-ssm-design-review-2026-09-12.md).
The device run supplies marker timing and host service measurements, not an automatic
proof of framebuffer reconstruction.

**Frame semantics answered by the author, 2026-09-12.** The question below is closed
and the earlier "next complete frame" default is withdrawn. Longshot's reply, verbatim:

> Amspirit crée le Snapshot sur la Vsync qui suit le code SSM. La notion de frame est à
> géométrie variable. Parfois c'est le moment ou le CRTC commence à construire une image
> (et donc il peut y avoir plusieurs frame au sein d'une image), et d'autres la situent
> par rapport à la Vsync (qui pourrait elle aussi avoir lieu plusieurs fois). Et surtout
> si tu as un écran de C4=0 à 38 (39x8=312), et que la Vsync se produit sur C4=30,
> l'image produite et le résultat de C4=0..29 du frame courant, et du C4=30 à 38 du
> frame précédent... Mais dans l'esprit, le screenshot DEVRAIT avoir lieu sur l'opcode.
> Si je veux qu'un screenshot ait lieu sur la VSYNC, je le mets sur la VSYNC

Asked whether the distinction is exercised anywhere in SHAKER, he answered that it is
not, and said why:

> Je ne pense pas que ça change quoi que ce soit, car j'ai placé les codes SSM sur des
> zones stables (ce qui est affiché est stable depuis plusieurs vsync). Il y a plusieurs
> types de tests dans SHAKER. Si on exclu les tests interactifs (prévus pour de la R&D
> qui est reportée dans le Compendium), il y a des tests qui affichent des résultats
> calculés et chiffrés (1) et des tests qui affichent des données sans qu'il soit
> possible de pour le Z80A de connaitre le rendu (2). La plupart des tests de type 2
> tournent en temps fixe et la vsync n'est donc plus testée par le code. Si le CRTC est
> mal émulé et que le compteur C4 est par exemple incorrectement calculé, alors la Vsync
> bouge et l'écran apparait désynchronisé, ce que le SSM capture. Sur certains tests ou
> il existe un "clignotement" entre 2 éléments graphiques, Shaker est paramétré pour
> faire 2 captures SSM permettant d'avoir les deux états.

He also notes that one AMSpiriT variant captures on the following VSYNC and the other
at the instruction.

### What the reply establishes

The intended trigger is the opcode, including ordinary per-test codes. Keep that as
the exact path's default. The author's stability statement describes the interval
**before** a marker; it does not guarantee unchanged output until an arbitrary SSH
capture, next VSYNC or complete later pass. Paired captures deliberately preserve
different states. AMSpiriT remains useful after checking each selected case, not because
all capture policies are proven equivalent for the entire corpus.

A persistent surface is a reasonable implementation model for the author's C4 example.
Its addressing, interlace mapping and initial contents are **our choices to validate**.
The reply does not specify a physical-line formula or a CRT phosphor/monitor model.
A framebuffer neither models phosphor decay nor proves what a monitor will lock to.

`frame` in format 1 is a VSYNC-edge counter only. Define capture coordinates using the
chosen observed sync stream; do not derive them from internal C4/C9 state and thereby
hide a displacement. Preserve the raw timing evidence used to interpret a rendered
image. Missing or repeated sync must remain observable rather than be normalized away.

### What a two-pass stitch can and cannot do

For a fixed progressive raster visited once in monotonically increasing address order,
`A[0..P) ++ B[P..end)` can reconstruct the persistent surface at P, provided A contains
the current prefix and B the immediately preceding complete pass. Freezing at the end
of that pass retains enough data for more than one earlier cut. This is a valid limited
optimization, not a reason to require three mirrored surfaces.

The old freeze-at-first-marker wording still cannot serve a later marker. Generalizing
the stitch also requires care: interlace visits alternating rows, and short/multiple
syncs can repaint an address after a marker. If the pre-marker value is overwritten,
neither a later stitch nor a pass stamp can recover it. Keep an ordered sample history
in the first prototype and let the host derive placement. A 4-bit pass stamp alone is
neither an intra-pass cut nor sufficient generation context over several seconds.

The author has not established a minimum paired-marker gap. Test both same-window and
cross-window cuts; do not assume two graphical states require two complete traversals.
A next-VSYNC comparison policy needs samples through that actual later edge, which may
lie beyond the window containing the opcode. Retaining only the opcode's window does
not automatically serve every policy.

### Selected observation boundary

Start with a **native, converted-colour capture**, before gamma, scandoubling, HQ2x
and crop, at the existing `amstrad_video_color` output inside
[`amstrad_video_output.sv`](../rtl/amstrad_video_output.sv). Retain 8 bits each of R/G/B
and the aligned HSync/VSync/HBlank/VBlank tuple from that same boundary. This preserves
the classic DAC lookup and Plus expansion. Four input bits per channel at the motherboard
are not four output intensity bits: classic low bits encode the GA level/OE pair.

The first exact-capture profile must explicitly apply native pixel cadence
(`pixel_rate_select = 0`, HQ2x off) and record the applied B6 mode and colour settings.
Do not force Raw CRT just to get native cadence: it also changes acquisition geometry.
Expose the existing converted tuple as an observation tap; do not recreate the colour
converter or add a competing display path. This is not a final-mixer/HDMI screenshot.
If later capture moves after the mixer, it must use that stage's actual `CE_PIXEL`,
24-bit RGB and timing together, with a new validated cadence/storage contract.

Extend the existing B6 fixture, including its completed final-RGB follow-up, to prove
the selected tap and timestamp. Phase 1's pre-conversion `hs/vs` stamp is insufficient.
Define one ordered sample index and a marker cut at HH fetch completion, accounting
for the detector's registered latency and a simultaneous pixel enable. State exactly
whether the coincident sample is included. The target is output present at the opcode
instant; do not delay the marker by an assumed CPU-to-video latency. Any alternate
alignment policy must be separately named and tested.

### Prototype: rotating windows of ordered native samples

Append RGB/timing samples sequentially into fixed-size time windows. **Do not address
DDR rows by sync in this prototype:** an extra VSYNC or repeated line must append
another sample, not overwrite earlier evidence. The host reconstructs sync-relative
rows and fields from the retained stream. Keep 32 bits per sample for RGB24, the four
aligned sync/blanking flags and a recorded field signal. Window generation plus sample
offset supplies ordering; no per-pixel pass stamp is needed.

A screenshot marker records its code, occurrence, tick/sample cut and the generations
of the current window and required preceding windows. Pin that set. Continue recording
until the current fixed-size window is sealed, then rotate to an available window.
This postpones transfer readiness while preserving the earlier cut exactly in the data.
Different markers may share windows with different cuts; later writes cannot destroy
those samples within an append-only window. Sync and non-screenshot reserved events
remain telemetry and do not request a capture set.

Choose the prehistory depth for a **measured, explicitly supported raster profile**.
Two arbitrary 20 ms windows do not guarantee a complete persistent image when a field
is longer, sync is absent, or some addresses have not been refreshed. Derive field
placement from measured HS/VS phase and retain the core's FIELD as a separate signal
for cross-checking. Establish progressive and interlace reconstruction independently;
B test 1 requires the latter. A moving but periodic VSYNC can be rendered relative to
the observed sync. Preserve the original stream so normalization cannot erase the
movement under investigation.

The host starts reconstruction with explicit invalid history, applies samples in time
order up to the cut, and classifies an image complete only when the profile's required
pixels/history are present. Otherwise retain the trace and report an incomplete image.
No finite prehistory can promise arbitrary never-refreshed framebuffer contents.
Missing sync needs a bounded diagnostic representation, not an invented rectangle.

### Retention and publication

Use a finite retention interval as the initial **candidate** in place of a release
mailbox. Select it from measurements; Fable's 5–10 seconds is a suggestion, not a
verified device budget. Windows referenced by a capture remain pinned until expiry;
without a release channel they remain pinned even if the host has already copied them.

1. Invalidate a window and change its generation **before** reusing its payload.
   While filling, it cannot be read as a valid sealed window.
2. Seal it only after its payload and any final packed word are written under the
   verified DDR/HPS ordering contract. A marker inside a packed word is a host cut;
   it does not require flushing the writer at the marker.
3. Publish the capture's window-generation list, cut, format/cadence, expiry and loss
   flags. The host reads each sealed descriptor, payload and descriptor again, accepting
   only unchanged generation **and sealed state**, all belonging to the same run.
   Generation equality alone cannot distinguish a stable header over partial writes.
4. On expiry, reuse is allowed and observable. A late reader gets a capture-loss
   result, never a silently mixed image. If all windows remain pinned, discard/report
   capture input or requests explicitly; never stall CPU/video to conceal exhaustion.
5. Reset/disable invalidates partial captures and requires the startup boundary again.
   Preserve bounded addresses, safe Avalon completion and an explicit FIFO overflow
   path. Do not combine old-run descriptors with new-run samples.

This deliberately offers bounded retention, not ownership until acknowledgment.
If measurements show it is too wasteful or unreliable, assess a small host-release
mailbox then. A mailbox, clone engine and general abandoned-host protocol are not
prerequisites for the first prototype. Existing ring writes and sample writes still
need one ordered DDR writer/arbitration path; passivity does not remove memory traffic.

### Sizing and service budget

At native 16 MHz and four bytes per sample, one stream writes **64 MB/s**: eight
million packed 64-bit writes/s. A 20 ms window would contain 320,000 samples and
1.28 MB (1.22 MiB). This is a wall-time storage example, not a definition of a frame,
a mandatory window size, or a complete-image bound. Explicit sample counts bound all
writes independently of missing sync.

The 64 MHz, 64-bit port's ideal 512 MB/s is only an upper bound. Measure sustained
service and worst stalls with scaler/HPS traffic; budget a small input FIFO and report
overflow as lost evidence. The native pre-conversion stamp's stage offset and the
64 MHz tick wrap also need the stated cut convention, not a guessed pixel correction.

Pool size depends on the required rolling prehistory, windows needed to continue writing,
and the maximum **unique windows pinned during the entire retention interval**. Shared
windows can reduce that count, but completed host copies do not release them early.
Fable's four/six-window examples are not accepted sizes until the marker pattern, hold
time and readback measurements fit. Reserve the entire interval, including ring and
descriptors, against framework/Main/kernel allocation before device use. If a proposed
hold time demands too much memory, shorten the supported interval or reassess explicit
release; do not silently weaken the loss guarantee.

### Pinned experimental implementation contracts

The first prototype may be implemented and simulated before the device gate. Production
instantiation is compile-time default off; enabling it requires a separately reviewed
memory allocation and an authorized build. Preserve the format-1 event ring for the
ordinary path. Keep the sample/capture ABI separately versioned so experimental captures
do not silently change the existing reader.

**Status 2026-09-12: a prototype implementing these contracts is in source and
uncommitted; final source verification is recorded in the review document.** `rtl/ssm_sample_recorder.v` is the recorder,
`rtl/ssm_ddr_arb.v` gives the DDR3 write port one owner, `rtl/amstrad_video_output.sv`
carries the observation tap, `docs/ssm-capture-abi.md` is the concrete ABI and
`scripts/hardware-loop/ssm_capture.py` the independent host decoder. The switch is
`SSM_SAMPLE_RECORDER`, undefined in `Amstrad.sv`, so the default build is the phase 1
build. Simulation parameter overrides live in `sim/ssm_recorder_top.v`.

- **Samples:** little-endian 32-bit words: R bits 7:0, G 15:8, B 23:16,
  HBlank 24, VBlank 25, HSync 26, VSync 27, aligned FIELD 28, reserved 31:29 zero.
  Pack the earlier sample in the low half of each 64-bit write.
- **Default bounds:** eight windows of `2^19` samples each (16 MiB payload), two
  preceding windows per cut, hold `2^28` core ticks (about 4.19 s at 64 MHz).
  Parameters/header fields describe these choices. They provide at least 65.536 ms
  of preceding sample history once filled at 16 MHz, not proof of complete SHAKER
  interlace reconstruction or sufficient retention at an arbitrary marker rate.
- **Cut:** latch the logical sample count and tick at the HH fetch-completion edge
  in the raw tap. A sample consumed on that same clock edge is excluded. Carry that
  timestamp through recognition; do not substitute the later registered `hit` time.
  Logical sample positions advance even on FIFO loss. Mark affected windows invalid
  rather than treating a compacted stream as continuous or locating gaps from a total
  count alone. Cuts at a window boundary must reference the last included sample's
  window and its preceding history, not accidentally pin only the new empty window.
- **Publication:** one ordered writer owns event, descriptor and payload traffic.
  Begin with single-beat writes and a bounded FIFO (64 packed words is an experimental
  default); burst optimization is optional after measurement. Invalidate a descriptor
  before payload reuse, seal only after all payload beats are accepted. This establishes
  source-level ordering; Avalon acceptance alone does not prove HPS visibility across
  the interconnect. Device acceptance must verify that ordering and read attributes.
- **ABI:** use explicit aligned fields and sufficient space. A fixed 64-byte capture
  record can retain the legacy 16-byte event prefix, current window/generation and cut,
  two separate 64-bit previous-window references, and status/identity fields. Do not
  pack two full 32-bit generations plus indices into one 64-bit word. Document byte
  offsets, endian order, valid counts, generations, lost/forced-expiry counters and
  initialization state alongside the independent host decoder before writing RTL.
  Descriptors may use 32 bytes to keep fields and publication unambiguous.
- **Lifecycle:** initialize every descriptor invalid before publishing recorder ready.
  Enable epochs distinguish runs within one FPGA configuration; they do not provide
  a globally unique identity across reconfiguration. The host discards prior state on
  a controlled reload and requires the bounded startup handshake. General attach,
  concurrent reload and persistence across loads remain unsupported. Do not add HPS
  writes to physical memory merely to manufacture freshness in this implementation.
- **Pool exhaustion:** prefer an unpinned/expired window; otherwise force-expire the
  oldest pinned window and count it. Reuse changes generation before payload, so every
  affected capture is detectably lost. Pin/reuse decisions on the same edge must have
  deterministic priority. New captures with missing history are explicitly incomplete.
  Keep recording without stalling the CPC. Capacity depends on unique retained windows
  and actual expiry; `(W-1)/(PIN_PREV+1)` is only a rough disjoint-set budget, not a
  guarantee for overlapping captures or a throughput bound.

The host decoder first supports a named measured/synthetic raster profile, with
progressive and interlace fixtures validated separately. Derive physical field placement
from HS/VS phase; preserve core FIELD as evidence. Unsupported phase, missing sync,
FIFO loss or insufficient history produces an incomplete result with the raw trace.
Keep the original samples and profile metadata even when emitting a reconstructed image.

### Implementation and acceptance gates

Before implementation, pin the tap, sample-cut convention, bounded window/prehistory
profile, memory map and sealed-descriptor ABI. Validate the candidate with a small
independently specified stream decoder and a few counterexamples:

- Different pixels before and after two markers in the **same** window, with host
  reads delayed until both have fired. Both reconstructed cuts show their own state.
- An address repainted twice inside one window, odd/even fields with distinct content,
  and missing/extra sync. Preserve ordered samples; insufficient prehistory must be
  visible, and periodic displacement must survive reconstruction.
- A marker inside a packed word while DDR is stalled; sealing follows the final payload
  write, and the host cut excludes later samples even though the sealed window includes
  them. No assumed pass end is required for a malformed raster.
- A host read racing window reuse, expiry, pool exhaustion and reset. Check sealed state
  as well as generation, including a reuse that starts before the first descriptor read.
- Detector off/on with the recorder stalled: CPU, CRTC and production video samples
  remain identical. Ring event loss and image loss must be independently visible.

The production B6 composition must supply the RGB/timing inputs; a toy raster alone
cannot close the interface gate. Run `make -C sim`, lint, host decoder tests and the
unchanged canonical soak for behavior-preserving observer changes, followed by fresh
cross-provider implementation review. Obtain synthesis/timing evidence through the
normal authorized workflow. Hardware acceptance requires repeated static captures and
both distinct flashing states, zero unexplained losses and preserved configuration.
Simulation does not establish HDMI/CRT behavior or SHAKERLAND agreement.

### Existing facilities and their limits

`HDMI_FREEZE` blanks RGB in `sys/video_mixer.sv`; `video_freezer` keeps sync alive.
It is not a way to retain the requested picture. Main's direct scaler-memory read is
also not an ownership handshake: triple buffering/interlace can race an HPS copy.
User-space latency cannot choose the HH instant in real time, but host software can
retrieve an immutable FPGA snapshot later. A Main change could carry that handshake;
it cannot replace the required retention mechanism with faster polling alone.

## Reference comparison

Use the code table to annotate captures **before claiming coverage**. An ignored JSON
export of the supplied workbook is sufficient; no runtime Excel dependency is required.
Record the workbook hash, disc/script version, code, occurrence and requested/applied
CRTC identity. The table's five blank test ids remain unknown. Unmatched or inapplicable
codes are diagnostics, not automatic proof that the wrong core configuration was loaded.

Resolve image associations through the portal's code table/API with missing references
reported explicitly. Preserve the source photograph and an unmodified capture. For
AMSpiriT comparisons record variant/version, timing policy, dimensions, colour settings
and any normalization. Do not auto-align away a sync displacement under investigation.
A useful emulator pixel comparison is not a hardware correctness verdict; Logon System
photographs and real hardware remain the authority.

## Remaining decisions and handoff

Recommended order: **DDR preflight and minimal phase 0/1 fixes, required implementation
review, authorized synthesis and bounded device gate, then the single-stream phase 2
device prototype**. Local implementation/simulation of the default-off phase 2 prototype
may proceed alongside phase 0/1 repairs under the pinned contracts above. Full
conformance follow-ups proceed as their features are exercised. The
full recorder follows only after the prototype demonstrates retained history, coherent
publication and a feasible retention/service budget. Reference annotation supports the
device gate rather than being deferred until all RTL is built.

Still open: 2.6-script compatibility with 2.7; the UM6845R 1A/1B characterization;
French-ROM shifted-digit menu behavior; safe DDR allocation and startup identity;
actual paired-state dwell and host latency; and phase 2 interlace/history limits,
window-retention capacity and sustainable bandwidth. Exact capture can proceed in a
documented subset without pretending those questions are all closed.

Implementation locations: Phase 0 uses `scripts/hardware-loop/csl_runner.py` and
`cpc_keys.py`; Phase 1 uses `rtl/ssm_marker.v`, the motherboard raw fetch tap,
`Amstrad.sv`, `ssm_ring.py` and their existing tests/build entries. Phase 2 adds the
observation tap to `rtl/amstrad_video_output.sv`, recorder/DDR arbitration and matching
host decoding/control, extending the existing B6 fixture. Keep classic/Plus behavior
unchanged; any actual behavior repair belongs in its own stream.

### Current implementation and verification

The prototype is implemented in `ssm_sample_recorder.v`, the shared production
`ssm_recorder_subsystem.v`, `ssm_ddr_arb.v`, the video observation tap and
`ssm_capture.py`. The opt-in `live` command uses the same decoder as offline captures
and exports raw samples, JSON identity/geometry/loss metadata and a cropped PPM.

Configuration changes reject coincident and subsequent samples/markers while old
accepted writes drain, then invalidate history and start a new epoch. The host requires
native, non-Raw, non-HQ2x cadence with the alternate pixel rate off; colour mix and
machine selection remain recorded metadata. Capture ABI v1 represents at most two
predecessors. Commit and descriptor identities bracket payload retrieval; source-level
ordering is not proof of atomic HPS visibility.

The [review record](csl-ssm-design-review-2026-09-12.md) owns final source acceptance and
gate results. Hardware allocation, ordering, throughput, measured raster profiles and
SHAKER photographic acceptance remain open.
