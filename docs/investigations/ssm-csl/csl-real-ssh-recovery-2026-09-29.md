# CSL recovery through an interrupted SSH transport

## Scope and method

This controlled experiment exercises the production host runner at base
`9ddbf74b35f4f5214f979d5039eb2961b536d121`. It targets failure after a CFG write
has reached the real MiSTer but before the SSH command completes. It does not
exercise a network-wide outage, a partial file write, or a core reload.

The one-shot harness is retained with raw local evidence under
`.fdc-scratch/csl-real-ssh/`. Before mutation it reads the actual 16-byte
`/media/fat/config/Amstrad.CFG` into a separate local backup. A second independent
SSH transport remains available for verification and emergency restoration.

The script selects the opposite CRTC CFG bit and resets. The harness intercepts
only the first CFG-write SSH subprocess, preserving production SSH options. The
remote write is followed by an acknowledgement and a bounded sleep. After
receiving the acknowledgement and independently reading changed bytes, the host
kills that exact SSH child. Its actual nonzero process status flows through
`SSHTransport.run_cmd` into the unmodified runner. No synthetic exception stands
in for a dropped connection. The runner should stop before core loading and
restore the original CFG through a fresh connection.

Acceptance requires a nonzero CLI return, a failed persisted manifest, successful
runner restoration, byte-identical local backup and remote CFG, and removal of
experiment staging files. Emergency restoration is separate from runner success:
if it is needed, the experiment cannot claim that runner recovery passed.

## Results

**PASS**, 2026-09-29 04:38:09–04:38:13 UTC. The coordinator granted the
exclusive device slot after task 1 restored its original configuration and
returned MiSTer to MENU. This task independently backed up that configuration.

- Original and restored bytes: `00004000040000000000000000000000`.
- Original and restored SHA-256:
  `13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
- Independently observed changed bytes: `04004000040000000000000000000000`.
- Remote acknowledgement: `CSL_WRITE_ACK`; interrupted local SSH PID: `29143`;
  actual process return code: `-9` (SIGKILL).
- Production CLI entry function `csl_runner.main()` returned `1`. Its persisted
  manifest status is `failed`, with reason
  `Cannot write /media/fat/config/Amstrad.CFG (-9): `.
- `cleanup.cfg_restore.attempted` and `matches_original` are both true. The
  runner's own local original backup also matches byte for byte.
- Emergency fallback was **not used**. The runner removed its staging file,
  `/media/fat/csl_cfg_1790656689_fb1148.bin`; independent absence checks passed.
- A later independent check found the original CFG hash, no staging file and
  no remaining process referencing the experiment's unique staging name.
  Device ownership was explicitly released to the coordinator.

The command was:

```sh
python3 .fdc-scratch/csl-real-ssh/experiment.py --device-slot-granted > .fdc-scratch/csl-real-ssh/live-run.log 2>&1
```

The harness exited 0 because its acceptance assertions passed; the invoked
production CLI entry function returned 1 because the interrupted write failed.
These are separate results. The full actual SSH argv, command timestamps and
independent reads are in [evidence.json](evidence/csl-real-ssh-2026-09-29/evidence.json).
The production [manifest](evidence/csl-real-ssh-2026-09-29/manifest.json) records
write failure followed by fresh SCP/SSH restoration and cleanup. The final
[release check](evidence/csl-real-ssh-2026-09-29/release-check.json) records the
hash and staging absence; unrelated process-list contents are omitted.

An initial invocation was rejected by CLI argument validation because the
harness supplied `/media/fat` instead of a path beneath it for `--disk-dir`.
It made no CFG write. The local evidence under `run-rejected-disk-dir/` confirms
unchanged original bytes and no fallback. The harness was corrected to
`/media/fat/games/Amstrad`, a path never reached by this interrupted run.

## Review and verification

Opus 5.5 medium preflight review `20260929T043040Z-16797-73eb` found two
harness defects: a reversed CRTC-bit toggle and acceptance checks that could
mistake a caught harness assertion for the intended transport failure. Both
were corrected before device contact. The final checks require actual `-9`,
changed bytes, CLI return 1 and the production write-error message together.

Follow-up review `20260929T043227Z-19163-d22f` returned **CLEAR**; its
[full output](evidence/csl-real-ssh-2026-09-29/opus-preflight-final.txt) is retained.
The subsequent disk-directory argument correction changes neither interception
nor recovery. Pillow was available and the harness passed syntax compilation.
No production code or RTL changes were necessary. This is a documentation and
evidence-only change, so the selected simulation gate is not applicable.

## Acceptance limits

This closes the real SSH-interruption recovery residual for a completed CFG
write followed by loss of its owned SSH client while fresh connections remain
available. It does not establish partial-write recovery on hardware, recovery
through a persistent network outage, server failure, active OSD interaction,
or loaded-core state restoration. No RBF was loaded and no Main command was
sent. MENU state was supplied by task 1; this task made no independent MENU
readback and no command that could change the loaded core. Offline
[partial-write and cleanup-failure coverage](csl-runner-safety-2026-09-29.md)
remains distinct. The CSL key-map hardware residual and wider Phase 1
acceptance are unchanged.

Local harness SHA-256: `eded788e6591b5694edf0d0d514a02aa6c582e6d6537249cfb752b8b7e976e6c`.
