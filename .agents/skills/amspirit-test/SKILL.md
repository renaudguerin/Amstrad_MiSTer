---
name: amspirit-test
description: Reproduce, probe, or capture CPC title behavior in the AmSpirit Lite emulator. Use the repository helper to load CPR/SNA media, drive input, inspect machine state, and compare AmSpirit evidence with MiSTer results.
---

# amspirit-test

Use AmSpirit as a diagnostic reference while investigating CPC and CPC Plus behavior.
The helper drives an existing AmSpirit Lite instance through its HTTP API. It does not
control MiSTer.

## Operating rules

- A user request to test in AmSpirit authorizes loading the requested CPR or SNA and applying
  the case settings. Use the existing instance even if another program is running; that alone
  is not a reason to ask before loading.
- Do not manually preserve and restore the prior machine state or config/render settings just
  for cleanup. Leave the requested test config/render settings applied and record them with
  the evidence. The helper's pause-state behavior is described in its README.
- Read [the helper guide](../../../scripts/amspirit/README.md) for commands, case format,
  supported API behavior, and limitations. Read [the oracle design](../../../docs/investigations/hardware-runs/amspirit-oracle-design-2026-09-13.md)
  before treating a result as evidence for a hardware rule.

## Run a test

1. Identify the title/media, desired machine configuration, and observable checkpoint from
   the request and current investigation. Reuse an existing case when it fits. Do not ask for
   details already present in the task or its handoff.
2. Query the instance and record its identity and current settings:

   ```sh
   python3 scripts/amspirit/amspirit.py identity
   ```

   The helper defaults to `http://127.0.0.1:6128`; pass `--url` before the subcommand to use
   another instance. If the API is unreachable and AmSpirit Lite is not running, launch it with
   its web server enabled:

   ```sh
   /Applications/amspirit-lite-sdl.app/Contents/MacOS/amspirit-lite-sdl --web-server
   ```

   Then rerun `identity`. Do not launch a second instance when the API is already reachable.
3. Prefer `amspirit.py run <case.json> --out-dir <new-directory>` for repeatable work. The
   output directory must not already contain a manifest. The case records configuration,
   render settings, media, and input steps. CPR loading needs a hard reset; SNA loading must
   not be reset. The helper applies the case settings and leaves them active.
4. Inspect the manifest, screenshots, and state dump at the recorded checkpoint. A repeated
   settled screenshot can be stale if the machine stopped producing VSYNC; identical frames
   on an animated title are a reason to inspect machine state, not evidence that the screen
   is stable. Use the helper's bounded frame waits rather than unbounded polling.
5. Keep decisive outputs under `docs/references/<topic>-<date>/` in the main checkout
   (gitignored, never committed). Record AmSpirit version, effective model/CRTC and render
   settings, media hash, input/checkpoint, and what the evidence establishes.

For a joint AmSpirit/MiSTer run, keep the case and media identity aligned but treat frame
counters as side-specific. Record each machine's effective configuration and image pipeline;
compare shapes and colours only after accounting for geometry and scaling. Use the
[`mister-capture` skill](../mister-capture/SKILL.md) for physical-device capture steps.

## Interpret results

An AmSpirit capture establishes what that AmSpirit build did in the tested configuration. It
does not establish that the FPGA core or physical hardware is correct. Emulator agreement is
diagnostic evidence; confirm consequential timing conclusions against original hardware,
Logon System reference photographs, or the relevant documented rule. For ACCC-derived CRTC
claims, the French compendium remains the documentary source.
