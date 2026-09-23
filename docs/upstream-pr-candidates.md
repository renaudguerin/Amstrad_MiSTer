# Upstream PR candidates

Fixes in this fork that could go to `MiSTer-devel/Amstrad_MiSTer`, with the case for each
and how to cut it. Each PR should be rebuilt against upstream's own files, not
cherry-picked: this fork's `rtl/sdram.v` has diverged (cartridge client, B8-7 write
latch, Sonic cartridge admission).

## Tape image collides with the CPC 464 model bank

- **Evidence:** hardware-confirmed on upstream `Amstrad_20260603.rbf` (identical crash
  frame to this fork); see
  [device record](investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md),
  "CDT on a CPC 464 overwrites the 464 OS ROM". Introduced upstream by PR #41
  (2026-05-09), which put the 464 on SDRAM bank 2 without moving the tape buffer.
- **Fork fix:** `859fd24`. Upstream only needs the `sdram.v` part: `TAPE_BANK = 2'b11`,
  `TAPE_BASE = 23'h100000`, and `tape_phys_addr = tape_addr + TAPE_BASE` used for the
  tape request address. The fork's vram-cache invalidation line only exists because
  of the B8-7 work below; drop it if upstream lacks that cache check.
- **Fix verified on hardware** (fork RBF `8b18ac0`, 2026-09-23): 464 ROM then CDT boots
  and loads AmstradDiag fully (same device record, "Device acceptance of the bank fix").
- **Case strength:** strong. It is a crash, reproducible on upstream's own release,
  and the change is a few lines in one file.
- **Known limit:** the tape window is 7 MB and not enforced; a larger image would wrap
  into the Dandanator/cartridge area of bank 3. Real CDTs are far below 1 MB, and a
  guard is not a one-liner (a dropped write still has to acknowledge, or the download
  stalls on `ioctl_wait`), so it is deliberately left out.

## Tape download write lifetime (B8-7)

- **Evidence:** simulation only. The physical-DQ SDRAM fixture showed duplicated
  writes, a next payload corrupting the previous byte, and the header being
  overwritten when the download ends before the grant; see
  [b8-7-tape-write-lifetime-2026-09-08.md](investigations/write-timing/b8-7-tape-write-lifetime-2026-09-08.md).
  No device run has shown the corruption: CDTs load fully on a classic 6128 on both
  upstream and this fork, probably because HPS byte cadence is slow enough that the
  race does not fire (not measured).
- **Fork fix:** `e7d73ba`: new `rtl/tape_write_queue.v` (plus its `files.qip` line),
  write acknowledge at `STATE_READ`, payload latched with the address in `sdram.v`,
  and `ioctl_wait` backpressure from `Amstrad.sv`.
- **Design:** sound. The module isolates the request/acknowledge contract so
  production and the fixture instantiate the same code; the contract is documented
  in its header. "Queue" is a generous name for a one-deep holding register.
- **Case strength:** weak on its own: a latent race with no user-visible symptom and
  a new file. Send it separately from the bank fix, if at all, and lead with the
  fixture evidence. A measured HPS cadence close to the race window would make it
  much stronger.
