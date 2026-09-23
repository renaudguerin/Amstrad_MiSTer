# Switchblade cartridge startup: redundant ASIC unlock zero — 2026-09-23

## Failure and first divergence

The unchanged commercial `Switchblade.cpr` is a well-formed 131,148-byte RIFF
with eight complete 16 KiB `cb00`–`cb07` pages (SHA-256
`d958e2b1eeebaa227aa33c4f0f5627fc238a2791fd177f7ebffefdf81a07ff78`).
On MiSTer build `8b18ac0`, RBF SHA-256
`26185f485ac72a6be7e3ee17f9fc2e57a2e09c595ba8061d53a9aca90c960da2`,
six serial native captures after a cold CPR load are entirely black and
byte-identical (768×273, SHA-256
`386c70a5e6243a33f73b8fbdea04993ca691ac5fd9bab6c79a6fc5ed78ba2bf6`).
The device media and RBF hashes match the local files. The test used explicit
6128+ / Full CFG (`13ef32c7…`) and restored the saved original CFG
(`2e585b4c…`) with matching device hash afterward. Native screenshots are
scaler-buffer evidence, not raw video or an instruction trace.

AmSpirit Lite 1.15.1 / core 2491682, 6128+ / CRTC3, boots those same CPR
bytes: its first frame is black, frame 100 shows the Amstrad intro, and frame
502 shows the Wild Blade logo. The frame-31 state has PC `0x2209`, SP `0xBFFA`,
lower RAM 0 and upper cartridge ROM 132. This is comparative emulator evidence,
not a claim about original CPC Plus hardware.

The production-T80 diagnostic with the old RTL copies cartridge banks into
RAM, reaches relocation, then executes the game's ASIC unlock sequence. After
its `7FA0` write and return to RAM address `0x0182`, it misses the expected
`JP 208F`, falls through sequential fetches and reaches a persistent `0x00EA`
loop by tick 35,457,154. The logged M1 address sequence proves the control-flow
divergence; early-edge logged data bytes are not sampled-opcode proof. The old
diagnostic binary's source had no relevant RTL difference from `b9edac2`, but
it is not a fresh build gate. Scratch trace: `/tmp/switchblade-exec-full.log`.

## Cause and bounded repair

At cartridge bank 0 offset `0x2011`, Switchblade sends:

```text
FF 00 00 FF 77 B3 51 A8 D4 62 39 9C 46 2B 15 8A CD EE
```

Arnold V §2.11 defines a nonzero/zero synchronization pair before the fixed
`FF 77 … 8A` prefix. The old `asic_unlock` detector starts matching at the
first zero, rejects the next zero while expecting `FF`, and never unlocks.
While locked, `plus_mmu` treats `A0` as legacy MRER rather than RMR2, enabling
lower ROM over the relocated RAM jump. The repair preserves synchronization
across redundant zeros only while awaiting the first fixed `FF`. It does not
change later mismatch handling, lock decisions, or reset/SNA restoration.
Acceptance of the extra zero is inferred from the original title's byte stream
and AmSpirit's behavior; Arnold V does not explicitly specify it.

## Verification and remaining acceptance

- Before the RTL change, the focused `make -C sim/plus run/asic_unlock_tests`
  failed on the title's redundant-zero prefix. After the change it passed all
  seven ASIC unlock cases, including a nonzero mismatch after the extra zero.
- After the behavioral RTL edit, `python3 sim/select_tests.py --run` reported:
  `select_tests: PASS 1 benches: run/asic_unlock_tests`. The existing
  `plus_p8_tests` also passed all 22 cases, including SNA sequence-state restore.
  Gemini bridge evidence: `/tmp/agents-roster-runs/20260923T054130Z-18640-0bc3/output.log`.
- A fresh changed-RTL production-T80 build running the **unchanged** CPR
  reaches PC `0x2209` at tick 35,239,538 with `unlocked=1`, instead of the old
  `0x00EA` loop. Scratch result: `/tmp/switchblade-current-trace.log`. This
  proves escape into expected early execution, not a rendered title.
- Fresh independent Astra review found no actionable defect. It identified
  one source-attribution wording issue, corrected in the RTL comment without
  changing behavior. That comment-only edit followed the simulation runs.

The [exact full-effort `41a1f27` MiSTer retest](../investigations/hardware-runs/plus-cartridge-originals-41a1f27-2026-09-23.md)
boots the original CPR through the intro, title graphics and recurring
high-score screen. Gameplay and audio remain untested.

Local capture, AmSpirit manifests and selected screenshots are retained under
ignored `docs/references/plus-load-2026-09-23/` in this task checkout.
