# Optional task-local MiSTer Batch Control build

Use an installed input tool when it already works. For a missing MBC binary, the
recipe below builds the existing public-domain upstream source without Docker,
global packages or changes to MiSTer. The resulting binary was executed successfully from a temporary `/tmp` path
on the MiSTer on September 12; see the [device record](b2-device-capture-2026-09-12.md).

Verified on Apple Silicon macOS, 2026-09-08. The pinned
[MBC source](https://github.com/pocomane/MiSTer_Batch_Control/blob/3873450d413c30e6b0339e6b3dbf2373e0e5a74a/mbc.c)
uses Linux uinput and libc. Its
[build instructions](https://github.com/pocomane/MiSTer_Batch_Control/blob/3873450d413c30e6b0339e6b3dbf2373e0e5a74a/Readme.md#installation)
use a static ARM hard-float target. Zig supplies a macOS-hosted C cross compiler
and musl; this recipe targets the DE10-Nano Cortex-A9 rather than the host CPU.
The [official Zig download index](https://ziglang.org/download/index.json)
provides the pinned archive checksum. Other hosts need their corresponding Zig
archive; the following download is specifically for Apple Silicon macOS.

From the repository root, with Python 3.12+ and curl available:

```sh
mkdir -p docs/references/b2-host-tools
cd docs/references/b2-host-tools
curl -fL --max-time 180 \
  https://ziglang.org/download/0.16.0/zig-aarch64-macos-0.16.0.tar.xz \
  -o zig-aarch64-macos-0.16.0.tar.xz
curl -fL --max-time 60 \
  https://raw.githubusercontent.com/pocomane/MiSTer_Batch_Control/3873450d413c30e6b0339e6b3dbf2373e0e5a74a/mbc.c \
  -o mbc.c
python3 - <<'PY'
import hashlib, pathlib, tarfile
checks = {
    'zig-aarch64-macos-0.16.0.tar.xz':
        'b23d70deaa879b5c2d486ed3316f7eaa53e84acf6fc9cc747de152450d401489',
    'mbc.c':
        'ae28316be83bd0b2db37cbe1c4f892285e4359e4649f0dbef36715c43d2d5c97',
}
for name, expected in checks.items():
    if hashlib.sha256(pathlib.Path(name).read_bytes()).hexdigest() != expected:
        raise SystemExit('Checksum mismatch: ' + name)
with tarfile.open('zig-aarch64-macos-0.16.0.tar.xz') as archive:
    archive.extractall('.', filter='data')
PY
# Apply the local repair described below before building for future captures.
python3 - <<'PY'
from pathlib import Path
p = Path('mbc.c')
s = p.read_text()
old = 'int i;\n    if (1 == sscanf(val, "%d", &i)) {'
new = 'char *end;\n    long i = strtol(val, &end, 10);\n    if (end != val) {'
if s.count(old) != 3:
    raise SystemExit('Expected exactly three wait-option parsing snippets')
p.write_text(s.replace(old, new))
PY
ZIG_GLOBAL_CACHE_DIR="$PWD/zig-cache" ./zig-aarch64-macos-0.16.0/zig cc \
  -target arm-linux-musleabihf -mcpu=cortex_a9 -std=c99 \
  -D_XOPEN_SOURCE=700 \
  '-DMBC_BUILD_COMMIT="3873450d413c30e6b0339e6b3dbf2373e0e5a74a"' \
  '-DMBC_BUILD_DATE="2026-09-08"' \
  -static -O2 -s -o mbc-arm-linux-musleabihf mbc.c
file mbc-arm-linux-musleabihf
shasum -a 256 mbc-arm-linux-musleabihf
```

The original, unpatched verified artifact is an `ELF 32-bit LSB executable, ARM, EABI5`, statically
linked and stripped; SHA-256
`0e99082bb8c9b8b2c2b6581c736dfc5d701a977a9c76fa548565cbc6642984db`.
The compiler, cache, source and binary remain under ignored `docs/references/`.
No ARM binary or toolchain is committed. A second local compilation produced the
same artifact hash with this exact source, flags and compiler.

## Known build-specific crash and local repair (2026-10-27)

The original `0e99082b…` artifact intermittently segfaults with environment
wait options set. GDB traced the failure through musl `vsscanf` to a Zig 0.16
`memchr` vector load crossing the top stack page in `read_options`; some
environment layouts do not crash. This finding is specific to that build.

The patch above replaces exactly the three `sscanf` wait-option snippets with
`strtol`, preserving invalid-value fallback and supported bounded wait values.
The repaired artifact has SHA-256
`f2b3a4f3cbd98c7a13f8770eb4c893bc352eae9d204f92f664f5eaad6c697330`.
The task-local source and binary are `.roster-scratch/mbc/mbc.c` and
`.roster-scratch/mbc/mbc-fixed`; the device copy is `/tmp/mbc-fixed`. The
original `/tmp/mbc` was left untouched.

The repaired build was checked with valid waits, invalid-wait fallback and a
release-only `raw_seq '}57'` returning exit 0. Actual CSL SHAKE27B key typing
and the CRTC 0 wait for SSM `00FC` also succeeded. The old build's environment
help crash was reproduced under GDB. No C source, binary or toolchain needs to
be committed.

On eventual device access, verify the selected target, executable permission,
`/dev/uinput`, and MBC input behavior before using it in a capture case. MBC's
`raw_seq` injects Linux key codes; it does not read the emulated keyboard matrix.
Use the driver's explicit MGL disk/cartridge mapping instead of MBC's built-in
Amstrad mapping. The French-ROM DSK input/capture smoke is recorded in the device report;
no permanent device installation is required.
