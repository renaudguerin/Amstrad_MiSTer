# Quartus UTM VM provisioning

This directory prepares the existing Debian 13 arm64 VM at `192.168.64.3`
for local Amstrad MiSTer builds. It restores `renaud`'s installer-created
groups, mounts UTM's Rosetta runtime, registers the x86_64 ELF handler,
enables Debian amd64 multiarch, and installs the native and amd64 runtime
dependencies used by Quartus and the repository's Verilator tests.

It deliberately does **not** download, copy, execute, or accept the license
terms for Altera's proprietary installer. No credentials or installer files
belong in this repository. Quartus Prime Lite needs no license file, but its
downloads and installer still have terms that a human must review and accept.

## Provision

From the repository root on the Mac:

```bash
cd ansible
ansible-playbook site.yml --check --diff
ansible-playbook site.yml
ansible-playbook site.yml --check --diff
ansible-playbook validate.yml
```

The first check-mode run skips resolution of amd64 packages because check mode
does not execute `dpkg --add-architecture amd64`. The real run enables the
architecture; subsequent check-mode runs can cover those packages. Some
runtime-only tasks, such as starting the systemd service, remain intentionally
skipped in check mode. The validation play executes Debian's amd64
`/usr/bin/hello` through Rosetta, then reports the Quartus version if Quartus
is installed. The final acceptance command uses `-e quartus_required=true` so
an absent toolchain fails rather than being mistaken for a complete build VM.

The inventory connects as root using the already-configured SSH key. It does
not configure passwords or copy private/public keys. If the VM address changes,
edit `inventory.yml` or override it temporarily:

```bash
ansible-playbook -i 'quartus-vm,' \
  -e ansible_host=192.168.64.3 -e ansible_user=root site.yml
```

## Human installer handoff

1. As a human, open Altera's official
   [Quartus Prime Lite 17.0 Linux archive](https://www.altera.com/downloads/fpga-development-tools/quartus-prime-lite-edition-design-software-version-17-0-linux),
   review its notices and license terms, and download only these files:

   | Purpose | Vendor filename | Published SHA-1 |
   | --- | --- | --- |
   | Base tools | `QuartusLiteSetup-17.0.0.595-linux.run` | `99ccfb15962febceba64de2dc9b28c47e5a3b8df` |
   | Cyclone V support | `cyclonev-17.0.0.595.qdz` | `2198dedb99866f38d43ff6c029d4bd668e2bbb59` |
   | Update 2 | `QuartusSetup-17.0.2.602-linux.run` | `cdc0389947ba6d3fb3206ac9840549c9fb38b093` |

   The archive warns that 17.0 is obsolete and lacks later functional and
   security updates. Keep this VM dedicated to trusted FPGA builds and use a
   current browser on the Mac for the download.
2. Copy the three files, unchanged and with their vendor filenames, into
   `/home/renaud/quartus-installer-17.0.2` in the VM. Do not put them in this
   repository. The Cyclone V `.qdz` must be beside the base installer so the
   installer can discover it.
3. From the Mac, verify that all three files are present and match the checksums
   published on the official archive page. This play is read-only; it neither
   runs an installer nor accepts a license:

   ```bash
   cd ansible
   ansible-playbook installer-preflight.yml
   ```

4. SSH into the VM as `renaud`, make the base installer executable, and launch
   it in interactive console mode (no X display is required):

   ```bash
   ssh renaud@192.168.64.3
   cd /home/renaud/quartus-installer-17.0.2
   chmod +x QuartusLiteSetup-17.0.0.595-linux.run
   export QUARTUS_CPUID_BYPASS=1
   ./QuartusLiteSetup-17.0.0.595-linux.run --mode text
   ```

   Review and accept the displayed terms yourself. Select
   `/home/renaud/intelFPGA_lite/17.0` as the installation directory and include
   Cyclone V support. An unattended invocation requires an explicit EULA-
   acceptance switch, so it is intentionally not scripted here.
5. Apply Update 2 interactively to the same installation directory:

   ```bash
   cd /home/renaud/quartus-installer-17.0.2
   chmod +x QuartusSetup-17.0.2.602-linux.run
   export QUARTUS_CPUID_BYPASS=1
   ./QuartusSetup-17.0.2.602-linux.run --mode text
   ```

   Review the update's displayed terms and confirm the existing
   `/home/renaud/intelFPGA_lite/17.0` installation. Do not create a second
   version directory.
6. From the Mac, apply the Rosetta compatibility patch to Quartus's environment
   script, then validate the result:

   ```bash
   cd ansible
   ansible-playbook post-install.yml --check --diff
   ansible-playbook post-install.yml
   ansible-playbook post-install.yml --check --diff
   ansible-playbook validate.yml -e quartus_required=true
   ```

   `post-install.yml` recognizes the two relevant sections of Quartus 17's
   `quartus/adm/qenv.sh` before changing anything, creates a one-time
   `qenv.sh.pre-rosetta` backup, selects the 64-bit tools on an aarch64 guest,
   and disables only the host SSE-probe section that cannot work through
   Rosetta. It then runs `quartus_sh --version` as `renaud` and requires
   version 17.0.2. Re-running the play makes no further edit. The final
   validation also rechecks the live Rosetta registration and amd64 execution.
7. Remove the three files from `/home/renaud/quartus-installer-17.0.2` when you
   no longer need the proprietary installer payload. The installed tools remain
   under `/home/renaud/intelFPGA_lite/17.0`.

Rosetta only translates 64-bit Intel Linux executables. Quartus synthesis uses
the 64-bit tools; legacy 32-bit utilities and USB/JTAG tooling are outside this
VM's supported build-only scope.

A container does not remove the human handoff: Altera's archive publishes
installers rather than a Quartus 17.0.2 container image, and a local image would
still have to be built from the same terms-governed files. The disposable VM is
already the isolation boundary, while direct Rosetta execution avoids another
amd64 userland and container runtime.

## Build as the normal user

Clone or copy this repository into the VM as `renaud`, then run:

```bash
build-amstrad /path/to/Amstrad_MiSTer
```

The wrapper always invokes `quartus_sh --flow compile Amstrad.qpf` from the
specified checkout and propagates Quartus's exit status. Output is written to
that checkout's `output_files/` directory. It does not use root privileges.
You can override the project filename with a second argument, although this
core's supported project is `Amstrad.qpf`.
