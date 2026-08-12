# Building the Amstrad MiSTer core

Target: synthesize `Amstrad.qpf`/`Amstrad.qsf` (top-level `sys_top`, Cyclone V, DE10-Nano)
into an `.rbf` and run it on real MiSTer hardware.

Your machine: **Apple Silicon Mac (arm64)**. This matters because Quartus is
x86_64-only, Windows/Linux-only, and there is no ARM build, ever. Everything
below routes around that one fact.

Repo facts (verified by reading the files, not assumed):
- `Amstrad.qpf` pins `QUARTUS_VERSION = "17.0"`.
- `Amstrad.qsf` header says `LAST_QUARTUS_VERSION "17.0.2 Standard Edition"`.
- There's also a legacy `Amstrad_Q13.qpf`/`.qsf` in the repo root targeting
  Quartus **13.1** — ignore it, it's an old/alternate project file, not the
  current target. Use `Amstrad.qpf`.
- `.gitignore` confirms build output lands in `output_files/` (rpt/sof/rbf
  all git-ignored) — this is a standard MiSTer project layout.

## 1. TL;DR recommendation

**Primary: GitHub Actions.** Zero local toolchain, zero emulation tax, free
for a public repo, and there's a proven example from another MiSTer core dev
(`srg320/Saturn_MiSTer`) using the `raetro/quartus:17.0` Docker image on a
plain `ubuntu-latest` runner. You already have this repo on GitHub. This is
the least-effort, least-risk path for a first build, and it's reusable for
every future change: push, wait for synthesis, then download the artifact.

**Fallback / for iteration speed: UTM VM (Debian arm64 + Rosetta 2) on the
Mac itself.** Counter-intuitively, this **beats Docker and beats a Windows
Server x86 box** in a real benchmark on M2 hardware (6m43s vs 8m10s Docker vs
10m03s Windows Server, same Cyclone V project) — see sources. Use this if you
want a tight local edit-compile-test loop instead of round-tripping through
CI on every change, or if you hit GitHub Actions' free-tier time limits.

Do **not** bother with: Docker Desktop running the amd64 Quartus image
directly on the Mac under Rosetta-in-Docker — it's slower than the VM route
above and has USB/JTAG passthrough problems (irrelevant here anyway since you
deploy to MiSTer via SD card / scp, not JTAG). Do not buy hardware for this —
CI is free and the VM fallback is free; a cheap x86 box or cloud VM is a
distant third option, listed below only for completeness.

## 2. Option A — GitHub Actions (recommended)

The checked-in workflow is `.github/workflows/build.yml`. It:

- runs for non-documentation pushes on every branch, non-documentation pull
  requests targeting `master`, and manual `workflow_dispatch` runs;
- installs Verilator on an Ubuntu runner and runs the aggregate CRTC/Plus
  behavioral tests and lint before allowing synthesis to start;
- runs checkout and artifact actions on `ubuntu-latest`, then compiles
  `Amstrad.qpf` by invoking the Quartus container with `docker run`;
- pins the `raetro/quartus` image digest currently corresponding to Quartus
  17.0.2.602, rather than trusting its mutable `17.0` tag;
- cancels an older run for the same workflow and Git ref when a replacement is
  queued;
- requires the generated RBF, fitter summary, and TimeQuest report after a
  successful Quartus invocation, so a missing timing report cannot be silently
  accepted; and
- uploads the available reports even when synthesis fails, plus a successful
  bitstream named `Amstrad_YYYYMMDD_<7-character-SHA>.rbf`. Dates are UTC.

The uploaded artifact is named
`Amstrad-build-<run-number>-<run-attempt>`, is retained for 14 days, and uses
`actions/upload-artifact@v4`. The workflow grants only read access to repository
contents.

Notes:
- `raetro/quartus:17.0` is amd64-only (verified via Docker Hub manifest), but
  GitHub's `ubuntu-latest` runners are amd64, so no emulation involved at
  all — full native speed on GitHub's hardware.
- The image is based on Debian Stretch. The workflow deliberately runs
  `actions/checkout@v4` and `actions/upload-artifact@v4` on the Ubuntu host,
  outside that old userspace; current JavaScript actions require a newer glibc
  than Stretch provides.
- Image is ~6GB, last pushed 2022 but still actively pulled (confirmed via
  Docker Hub API), and another MiSTer core (`srg320/Saturn_MiSTer`) has
  workflows configured to use it. This repository's first live run is the
  reliability and runner-disk-space canary.
- This repo's own project file is `Amstrad.qpf` (not `Amstrad_Q13.qpf`) —
  the checked-in workflow targets the right one.
- Actual duration and billing depend on the repository's current GitHub plan,
  visibility, runner availability, and the synthesis result. The workflow has a
  90-minute timeout; this is a safety limit, not an estimated build duration.
- Push, then watch the run with `gh run watch` or in the Actions tab. Download
  the artifact with `gh run download` or via the web UI.

```bash
git add .github/workflows/build.yml
git commit -m "Add CI build workflow"
git push
gh run watch   # or: open the Actions tab in the browser
gh run download --name "Amstrad-build-<run-number>-<run-attempt>" \
  --dir /tmp/amstrad-build
```

## 3. Option B — Local VM (Debian arm64 + Rosetta 2 via UTM)

Best local option on Apple Silicon; faster than Docker in practice because
Rosetta 2 translates x86_64 instructions directly rather than emulating a
whole different CPU, and Quartus's actual CPU-bound synthesis work benefits
from that more than from virtualization overhead.

1. Install UTM (`brew install --cask utm`).
2. Create a VM: Apple Virtualization Framework backend, Debian 12 arm64
   netinstall ISO, **enable Rosetta** in the VM's sharing settings (UTM
   exposes this as a checkbox on Virtualize-type VMs).
3. Inside the Debian VM: install `binfmt-support`, mount the Rosetta share
   UTM provides and register it as a binfmt handler, then
   `dpkg --add-architecture amd64` and install the amd64 runtime libs
   (`libc6`, `libbz2-1.0`, `libglib2.0-0`, `libgtk2.0-0`, `libcrypt1`,
   `libusb-1.0-0`). Exact commands are share-path dependent — full recipe in
   the gist under Sources.
4. Download **Quartus Prime Lite 17.0.2** (Linux) — see section 6 — and run
   the `.run` installer inside the VM, selecting Cyclone V device support.
5. Build: `cd ~/Amstrad_MiSTer && quartus_sh --flow compile Amstrad.qpf`
6. Copy `output_files/Amstrad.rbf` out via the UTM shared folder or `scp`.

Give the VM 16GB RAM if the host can spare it (`sysctl hw.memsize` to check)
— Cyclone V fitting is memory-hungry, 7-8GB peak is typical for cores this
size. More setup effort than Option A, but gives a tight local loop once
you're actively iterating on RTL rather than doing a one-off build.

## 4. Option C — Docker Desktop on the Mac (works, but slower — skip unless you have a reason)

`hunson-abadeer/MiSTer-docker-build` is explicitly developed/tested on macOS
and wraps `raetro`'s Quartus image with convenience scripts:

```bash
brew install --cask docker   # or ensure Docker Desktop is running
git clone https://github.com/hunson-abadeer/MiSTer-docker-build
cd MiSTer-docker-build
docker build -t mister-quartus .      # builds an amd64 image; Rosetta-in-Docker
                                        # emulation handles the arch mismatch
cd /Users/renaudg/code/Amstrad_MiSTer
../MiSTer-docker-build/mister_quartus_compile.sh Amstrad
```

Requires ~24GB Docker-allocated disk and ~7GB RAM at compile time (raise
Docker Desktop's resource limits accordingly). Output lands in
`Amstrad_MiSTer/output_files/Amstrad.rbf`. This is a real, working path — it's
just the slowest of the three based on the M2 benchmark cited in Option B's
source (8m10s Docker vs 6m43s Rosetta-VM vs 10m03s bare Windows Server for a
comparable Cyclone V project). Use it only if you don't want to touch UTM
and don't want CI round-trips.

## 5. Option D — Cheap x86 box or cloud VM (fallback of last resort)

If GitHub Actions minutes run out and you don't want a VM on the Mac:

- **Cloud VM**: Hetzner CX42 (8 vCPU/16GB) is ~€0.03/hr, €16.40/mo cap —
  spin up Ubuntu, install Quartus natively (no emulation), build, tear down.
  AWS `c5.2xlarge` equivalent runs ~$0.30-0.35/hr on-demand, pricier for a
  one-off. Treat as backup only — this replaces a free CI run with a
  per-hour cost.
- **Physical box**: any spare x86_64 mini PC/laptop with 16GB RAM running
  Ubuntu works; not worth buying new just for this.

Install steps are the standard native Quartus install (Option E below) — no
Docker, no VM, no emulation tax.

## 6. Getting the Quartus 17.0.2 installer itself

Intel/Altera still serves 17.0.x through the **archives** section of the
FPGA download center — it's not on the front page (current front-page
version is 25.x/26.x), but it hasn't been pulled:

- Landing page: https://www.intel.com/content/www/us/en/software-kit/669557/intel-quartus-prime-lite-edition-design-software-version-17-0-for-windows.html
- Full archive browser (pick version 17.0, edition Lite, platform
  Linux/Windows): https://fpgasoftware.intel.com/?edition=lite

Download the **Lite Edition** (free, no license file needed for Cyclone V)
plus the **Cyclone V device support** package — don't grab the full
all-devices bundle, it's tens of GB you don't need. Requires a free
Intel/Altera account to download (standard, not a paywall).

If Intel ever pulls the 17.0.2 archive, the `raetro/quartus:17.0` Docker
image (Option A/C) and community mirrors referenced in the MiSTer forum are
the fallback source — the installer bits are also baked into that image if
you need to extract them.

## 7. Deploying the .rbf to your MiSTer

Amstrad is a "computer" class core. Convention (confirmed against this
repo's own `releases/` folder, which already contains files like
`Amstrad_20260603.rbf`):

```bash
scp output_files/Amstrad.rbf \
    root@<mister-ip>:/media/fat/_Computer/Amstrad_$(date +%Y%m%d).rbf
```

- Default MiSTer SSH credentials: user `root`, password `1` (change this on
  your actual unit if you haven't).
- `_Computer/` is the standard MiSTer folder for computer-class cores; the
  update scripts and menu both recognize `CoreName_YYYYMMDD.rbf` there.
- Also copy `boot.rom` from this repo's root into `/media/fat/Games/Amstrad/`
  on the SD card if it isn't already there (required per this repo's
  README — the core won't boot without it).
- On the MiSTer: from the main menu, the core should now show up under its
  own name in the core list (reads `_Computer/` automatically). Load it,
  and if you need to force a clean reload of a previously-loaded core,
  Alt+F12 resets to the original ROM per the README.
- Alternative to scp: drop the `.rbf` onto the SD card directly via a
  card reader into `_Computer/`.

## 8. Sanity checks — how do you know the build is good

- **Compile must reach "Full Compilation was successful"**. `quartus_sh
  --flow compile` returns non-zero on failure — check `$?` in scripts/CI.
- **Fitter summary** (`output_files/Amstrad.fit.summary`): confirm device is
  Cyclone V, resource utilization not pegged at 100% (this core has
  historically fit comfortably).
- **Timing (TimeQuest) report** (`output_files/Amstrad.sta.rpt`): the `.qsf`
  sets `TIMEQUEST_MULTICORNER_ANALYSIS OFF`, so single-corner timing only —
  that's intentional for this repo, not a build error. A *small* number of
  setup violations in the tens-of-ps range on non-critical paths is normal
  for MiSTer cores generally; large (multi-ns) violations on clock or
  memory-interface paths are the ones to worry about. The `.sdc` also
  explicitly relaxes timing on the floppy controller's (`u765`)
  image-track-offset/index paths via `set_multicycle_path` — don't be
  alarmed by how TimeQuest reports those, it's deliberate.
- **RBF actually generated**: `GENERATE_RBF_FILE ON` is already set, so a
  successful compile always produces `output_files/Amstrad.rbf` — if compile
  succeeds but the file's missing, something is misconfigured, not normal.
- **On real hardware**: boots to the Amstrad BASIC/CPC boot screen with
  `boot.rom` present; load a known-good `.dsk` and confirm `cat` lists
  files — proves the disk I/O path works, not just that the bitstream
  loaded.

## 9. Sources

- [Compiling for MiSTer — official docs](https://mister-devel.github.io/MkDocs_MiSTer/developer/mistercompile/) — confirms 17.0.2 is the standard, notes Apple Silicon toolchains don't run natively
- [Quartus Prime Lite 17.0 for Windows — Intel/Altera](https://www.intel.com/content/www/us/en/software-kit/669557/intel-quartus-prime-lite-edition-design-software-version-17-0-for-windows.html)
- [Intel FPGA Software Download Center (archives)](https://fpgasoftware.intel.com/?edition=lite)
- [raetro/sdk-docker-fpga](https://github.com/raetro/sdk-docker-fpga) — the `raetro/quartus` Docker image family, versions 13.0-23.1
- [srg320/Saturn_MiSTer test-build.yml](https://github.com/srg320/Saturn_MiSTer/blob/master/.github/workflows/test-build.yml) — real working GitHub Actions workflow using `raetro/quartus:17.0`
- [hunson-abadeer/MiSTer-docker-build](https://github.com/hunson-abadeer/MiSTer-docker-build) — macOS-tested Docker wrapper scripts
- [Running Quartus + Questa on Apple Silicon (gist)](https://gist.github.com/federunco/f2bde2e25342c6284b68ce4ecf305e5d) — UTM/Rosetta method and M2 benchmark numbers (6m43s VM vs 8m10s Docker vs 10m03s Windows Server)
- ["Any way to develop on Mac? What tool options are there" — MiSTer FPGA Forum](https://misterfpga.org/viewtopic.php?t=9415)
- [Folders and File naming — Main_MiSTer wiki](https://github.com/MiSTer-devel/Main_MiSTer-wiki/wiki/Folders-and-File-naming) — `_Computer/CoreName_YYYYMMDD.rbf` convention, `Amstrad_20190923.rbf` cited as the literal example
- [Hetzner Cloud pricing](https://www.hetzner.com/cloud/cost-optimized) — CX42 8vCPU/16GB reference cost
- Repo files read directly: `Amstrad.qpf`, `Amstrad.qsf`, `Amstrad_Q13.qsf`, `Amstrad.sdc`, `.gitignore`, `README.md`, `releases/`
