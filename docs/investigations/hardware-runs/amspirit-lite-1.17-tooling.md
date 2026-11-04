# AmSpirit Lite 1.17 diagnostic tooling

Documentation checked 2026-11-03 against upstream
[Lite 1.16-to-1.17 migration guide](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/94c101bfab15039c89187a54d99fa8fb5322aba3/docs/lite/migration-1.16-to-1.17.md),
[Web API](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/94c101bfab15039c89187a54d99fa8fb5322aba3/docs/lite/web_api.md),
[scripting](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/94c101bfab15039c89187a54d99fa8fb5322aba3/docs/lite/scripting.md), and
[MCP emulator README](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/94c101bfab15039c89187a54d99fa8fb5322aba3/tools/amspirit_lite_mcp/README.md).
Installed binary on macOS: AMSpiriT Lite 1.17.1 (SDL2, Core V260522).
This document supersedes the active operational guidance of the
[Lite 1.16 tooling guide](amspirit-lite-1.16-tooling.md) (preserved as a historical record).
Emulator observations retain the [oracle evidence limits](amspirit-oracle-design-2026-09-13.md).

---

## 1. Protocol and Command Contract Changes (1.16 to 1.17)

### Command responses return `{"cmd_seq": N}` without `"ok"`
In 1.16, mutating `POST` and `DELETE` requests answered `{"ok": true, "cmd_seq": N}`.
In 1.17, successful command responses answer **`{"cmd_seq": N}`**. The `"ok"` boolean is no longer
emitted on success. HTTP status 200 signals acceptance into the command queue. Client errors return
HTTP 400 with a structured body `{"error": "...", "field": "..."}`.
The helper accepts both 1.16 and 1.17 responses: if `"ok"` is present, it must be truthy; if absent,
HTTP 200 acceptance is trusted.
This response compatibility does not make the complete helper compatible with
1.16: native joystick and snapshot streaming require 1.17 endpoints.

### Execution control consolidated on `POST /api/run`
Pause, resume, reset, and single-stepping have moved from `POST /api/config` to `POST /api/run`:
- Pause: `POST /api/run {"action": "pause"}`
- Resume: `POST /api/run {"action": "resume"}`
- Hard reset: `POST /api/run {"action": "reset", "kind": "hard"}`
- Soft reset: `POST /api/run {"action": "reset", "kind": "soft"}`
- Stepping: `POST /api/run {"action": "step", "target": "basic", "over": true}`
  (`target` can also be `"z80"`).
Sending `paused`, `do_soft_reset`, or `do_hard_reset` to `POST /api/config` is deprecated.

### Asynchronous command failure detection via `emu.failed_cmds`
Commands accepted by the web server thread may be refused when the emulation thread applies them.
While polling `GET /api/ping` for `emu.applied_cmd_seq >= cmd_seq`, the helper inspects `emu.failed_cmds`.
If an entry matches `cmd_seq`, an `OracleError` is raised immediately with the failure description.

### Model naming in configuration
`GET /api/config` and `POST /api/config` use string model names: `"464"`, `"664"`, `"6128"`,
`"6128+"`, `"464+"`, `"GX4000"`. Numeric model identifiers are legacy. The helper transparently
normalizes legacy integers to strings during requested vs. applied comparisons.

### Direct snapshot streaming via `GET /api/snapshot`
In 1.16, saving a `.sna` snapshot required Lua `snapshot()` scripting which wrote to `fs.root()` on the
host filesystem, requiring directory preparation and same-machine co-location.
In 1.17, `GET /api/snapshot` streams the `.sna` snapshot bytes directly in the HTTP response body
(`application/octet-stream`). This eliminates Lua scripting for snapshots and enables remote snapshot capture.

### Native joystick control via `POST /api/joystick`
In 1.16, joystick 0 was simulated by injecting keyboard matrix row 9 values over Lua `keyboard_write`
in a coroutine loop.
In 1.17, `POST /api/joystick` provides a dedicated endpoint on port 0:
```json
{"fire1": true, "frames": 10}
```
Directions (`up`, `down`, `left`, `right`) and buttons (`fire1`, `fire2`, `fire3`) are held for
the specified `frames` count and automatically released by the emulator.

---

## 2. Architectural Evaluation: MCP vs Direct HTTP for Amstrad_MiSTer

Upstream 1.17 provides an experimental Model Context Protocol (MCP) server
(`tools/amspirit_lite_mcp`). We evaluated whether to adopt MCP or retain direct HTTP for the
Amstrad_MiSTer test automation and oracle harness.

| Feature / Requirement | Direct HTTP Helper (`amspirit.py`) | Upstream MCP Server (`tools/amspirit_lite_mcp`) |
|---|---|---|
| **Runtime dependencies** | Python 3 standard library only (`urllib.request`) | Requires virtualenv, `pip install -r requirements.txt`, MCP SDK |
| **Media loading (CPR, SNA)** | Supported (`POST /api/media` with CRTC retention) | **Out of scope** (explicitly omitted in MCP design) |
| **CRT & Render configuration** | Supported (`POST /api/render`, presets, calibration) | **Out of scope** |
| **Scripting engine (CSL / Lua)** | Supported (`POST /api/eval`, interactive inspection) | **Out of scope** |
| **Execution control** | Supported (`POST /api/run`, bounded deadline waits) | Supported (`emu_control`, `z80_step`, `basic_step`) |
| **State inspection & memory** | Supported (`GET /api/state`, `/api/ping`, etc.) | Supported (`emu_state`, `ram_read`, `ram_write`, etc.) |
| **Settled screenshot capture** | Supported (`GET /api/screenshot`, hash validation) | Supported (`screenshot`) |
| **Automated case execution** | Batch execution with `manifest.json`, origin offsets | None (designed for ad-hoc interactive chat queries) |
| **Target execution model** | Headless CLI automation & test runners | Interactive LLM tool client (e.g., Claude Desktop) |

### Recommendation
**Retain Direct HTTP as the primary automation and oracle harness.**
The upstream MCP server is explicitly intended for interactive homebrew debugging with an attached LLM
client. It deliberately excludes media loading, CRT/monitor rendering, and Lua scripting. Because the
Amstrad_MiSTer workflow depends on automated CPR/SNA loading, CRTC model switching, settled frame captures,
and batch manifest generation, MCP cannot replace the HTTP helper. Direct HTTP provides complete capability,
zero external dependencies, and deterministic command sequencing.

---

## 3. Operational Notes and Environment Constraints

1. **Host Execution & Display Server**:
   `amspirit-lite-sdl` requires an active macOS WindowServer connection (`_RegisterApplication`)
   for SDL2 display initialization. In headless or sandboxed daemon sessions (such as seatbelt-isolated
   runner processes), the Cocoa backend aborts with `SIGABRT` (exit code 134). Launch the emulator
   from an interactive Aqua desktop session when live API probing is required:
   ```sh
   /Applications/amspirit-lite-sdl.app/Contents/MacOS/amspirit-lite-sdl --web-server
   ```
2. **Deterministic Test Verification**:
   The test suite in `scripts/amspirit/test_amspirit.py` covers the helper's 1.17 protocol contracts
   (response shapes, `POST /api/run` actions, `GET /api/snapshot` streaming, `POST /api/joystick` inputs,
   and `emu.failed_cmds` detection) deterministically without requiring a live display.

3. **Verified desktop launch and live smoke (2026-11-03)**:
   Direct launch from the restricted worker aborted, but launching the installed
   app through macOS succeeded with the platform's approval:
   `open -a /Applications/amspirit-lite-sdl.app --args --web-server`.
   Live `/api/ping` reports frontend `lite`, version `1.17.1`, core `2491682`;
   record those runtime fields rather than inferring the core identity from
   the executable's version banner. The complete Copter case produced the
   title, fire-driven options menu, and a downloaded version-3 Plus SNA.
   Evidence: `local/overnight-20261103/amspirit-copter-before/` (task checkout).
   Its legacy numeric monitor was accepted as Off but reported a naming
   mismatch; the maintained case now requests `"monitor":"Off"` explicitly.
   A desktop launch needs the local app/desktop to remain available; this
   does not establish unattended recovery after app exit or host sleep.

## Verification

The new migration checks failed against the original helper (3 failures,
4 errors). The accepted helper has 21 protocol/ordering tests; the identity
pass-through check was omitted because it merely echoed its fixture. Parent
OpenAI review of the Gemini-authored diff covered the documented response and
failure contracts, followed by the live title/options/snapshot smoke above.
After the last helper/case edit, the combined gate was:

```text
python3 sim/select_tests.py --run
select_tests: PASS 4 benches: test, crtc-test, crtc-cpu-phase-test, ga40010-test
```

Its helper output reports `Ran 21 tests` and `OK`; the same run includes the
separately reviewed classic E/P repair. Full output is ignored under
`local/overnight-20261103/selected-tests-final.log`. No claim of a live
apply-time refusal or of every upstream endpoint being exercised is made.

### Breakpoint replacement resumes execution on 1.17.1

On core2491682, `PUT /api/breakpoints` resumes a paused machine for both
empty and nonempty replacement sets. The empty-set control read
`paused=true` before the call and `paused=false` after application. The
nonempty control installed `B00:1891` at paused PC2084/tick29204307,
without a resume request; the machine reached PC1891/tick29218913.
These are observed frontend semantics, not an inference from elapsed host time.

A target can be hit while the replacement command is being acknowledged.
Unconditionally resuming afterward skips that first stop. For an ordered
checkpoint chain, acknowledge the replacement and inspect pause/PC first:
accept an already-paused target; explicitly resume only if execution remains
paused at the previous stop. If it is running, wait for the target. Use a
host deadline and the intended stop condition, rather than requiring a
resume observer to see `paused=false`.

For cleanup, clear breakpoints first, then explicitly pause and verify the
final state. Evidence below the ignored task-local root:
`classic/breakpoint-clear-pause-control.json`,
`classic/nonempty-breakpoint-pause-control.json`, and the corrected
`asic/r0-first-loop-amspirit-ordered/`. The earlier unconditional-resume
probe skipped the first E54 stop; its next-loop counters are not a first-loop
golden.

The core's streamed classic SNA defaults to version2 with zeros at the
v3 CRTC-counter offsets; those zero bytes are **not counter measurements**.
Plus snapshots observed in the same build are version3. Check the version
before interpreting offsets A9–B0 or claiming an instruction-start C0.

The pinned upstream [scripting guide](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/94c101bfab15039c89187a54d99fa8fb5322aba3/docs/lite/scripting.md)
explicitly lists `snapshot_version` as accepted and ignored. It cannot force a
classic v3 save. GET `/api/snapshot` also publishes no version parameter in the
live endpoint schema. Preserve a genuine pre-initialization CPU/RAM checkpoint
when later-state replay would otherwise rerun destructive initialization.
