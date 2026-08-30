# Quartus UTM VM provisioning

This directory prepares an existing Debian 13 arm64 UTM guest for local Amstrad
MiSTer builds. It restores the build user's installer-created groups, mounts
UTM's Rosetta runtime, registers the x86_64 ELF handler, enables Debian amd64
multiarch, and installs the native and amd64 runtime dependencies used by
Quartus and the repository's Verilator tests.

The playbook removes Debian's older Verilator package, builds Verilator 5.050
from its exact official upstream commit, and links it at
`/usr/local/bin/verilator`. `validate.yml` checks both the reported version and
the source-commit marker, so the local VM and GitHub-hosted simulation runner
cannot silently drift apart.

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
`/usr/bin/hello` through Rosetta, requires the pinned Verilator version, then
reports the Quartus version if Quartus is installed. The final acceptance
command uses `-e quartus_required=true` so an absent toolchain fails rather
than being mistaken for a complete build VM.

## Addressing the guest

`inventory.yml` targets `quartus-vm.local`, not an IP address. `site.yml` sets
the guest hostname and installs `avahi-daemon`, so the guest publishes itself
over mDNS and a new DHCP lease never has to be looked up on the console. macOS
resolves `.local` names natively.

Bootstrapping is the one exception: before the first `site.yml` run the guest
has neither the hostname nor avahi, so pass its current address once. Run
`ip -4 addr show scope global` on the guest console to read it.

```bash
ansible-playbook -i 'quartus-vm,' \
  -e ansible_host=<current-guest-ip> -e ansible_user=root site.yml
```

The inventory connects as root using an already-configured SSH key. It does not
configure passwords or copy private/public keys.

`site.yml` also sets the console font to Terminus 16x32, because UTM's default
framebuffer resolution renders the stock 8x16 font unreadably small.

The build user defaults to `admin` (`quartus_user` in `group_vars/all.yml`).
Every playbook derives paths from that variable, so a guest using a different
account only needs `-e quartus_user=<name>`.

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
   `/home/admin/quartus-installer-17.0.2` in the VM. Do not put them in this
   repository. The Cyclone V `.qdz` must be beside the base installer so the
   installer can discover it.
3. From the Mac, verify that all three files are present and match the checksums
   published on the official archive page. This play is read-only; it neither
   runs an installer nor accepts a license:

   ```bash
   cd ansible
   ansible-playbook installer-preflight.yml
   ```

4. SSH into the VM as `admin` and launch the base installer in interactive
   console mode through QEMU (no X display is required):

   ```bash
   ssh admin@quartus-vm.local
   cd /home/admin/quartus-installer-17.0.2
   export QUARTUS_CPUID_BYPASS=1
   /usr/bin/qemu-x86_64 ./QuartusLiteSetup-17.0.0.595-linux.run --mode text
   ```

   The old self-extracting launcher has a static ELF layout that Apple's Linux
   Rosetta loader rejects with `bss_size overflow`. `site.yml` therefore
   installs `qemu-user` without its binfmt recommender, and this command uses
   QEMU only for the launcher. Rosetta remains the registered x86_64 handler
   for the installed Quartus tools.

   Review and accept the displayed terms yourself. Select
   `/home/admin/intelFPGA_lite/17.0` as the installation directory and include
   Cyclone V support. An unattended invocation requires an explicit EULA-
   acceptance switch, so it is intentionally not scripted here.
5. Apply Update 2 interactively to the same installation directory:

   ```bash
   cd /home/admin/quartus-installer-17.0.2
   export QUARTUS_CPUID_BYPASS=1
   /usr/bin/qemu-x86_64 ./QuartusSetup-17.0.2.602-linux.run --mode text
   ```

   Review the update's displayed terms and confirm the existing
   `/home/admin/intelFPGA_lite/17.0` installation. Do not create a second
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
   Rosetta. It then runs `quartus_sh --version` as `admin` and requires
   version 17.0.2. Re-running the play makes no further edit. The final
   validation also rechecks the live Rosetta registration and amd64 execution.
7. Remove the three files from `/home/admin/quartus-installer-17.0.2` when you
   no longer need the proprietary installer payload. The installed tools remain
   under `/home/admin/intelFPGA_lite/17.0`.

Rosetta only translates 64-bit Intel Linux executables. Quartus synthesis uses
the 64-bit tools; legacy 32-bit utilities and USB/JTAG tooling are outside this
VM's supported build-only scope.

A container does not remove the human handoff: Altera's archive publishes
installers rather than a Quartus 17.0.2 container image, and a local image would
still have to be built from the same terms-governed files. The disposable VM is
already the isolation boundary, while direct Rosetta execution avoids another
amd64 userland and container runtime.

## Build as the normal user

Clone or copy this repository into the VM as `admin`, then run:

```bash
build-amstrad /path/to/Amstrad_MiSTer
```

The wrapper always invokes `quartus_sh --flow compile Amstrad.qpf` from the
specified checkout and propagates Quartus's exit status. Output is written to
that checkout's `output_files/` directory. It does not use root privileges.
You can override the project filename with a second argument, although this
core's supported project is `Amstrad.qpf`.

## GitHub Actions self-hosted runner

`local-runner.yml` registers the same VM as a self-hosted GitHub Actions
runner, so the local hardware is driven through the normal GitHub workflow
surface instead of hand-invoking `build-amstrad`:

```bash
gh workflow run local-build.yml --ref <branch-or-tag> -f effort=full
```

Watch and download artifacts exactly like a hosted run (`gh run watch`,
`gh run download --name Amstrad-local-build-...`). The workflow is dispatch-only:
starting it requires write access to `renaudguerin/Amstrad_MiSTer`, and its job
guard rejects every other event. That protects this workflow, not the runner
label globally: a pull request that rewrites an existing workflow could target
`quartus-vm` if someone approves that run. The disposable, dedicated VM is the
containment boundary. Keep host SSH keys, shared folders, repository secrets,
and unrelated credentials out of it; use NAT rather than a bridged interface;
and do not approve untrusted workflow changes while the VM is online. The
runner necessarily stores its own registration credentials and receives an
ephemeral read-only token per job. Keep `local-build.yml` dispatch-only when
editing it.

### Provision

1. In GitHub: Settings -> Actions -> Runners -> New self-hosted runner, pick
   **Linux arm64**, and copy the registration token (expires in about an hour;
   always mint a fresh one per invocation).
2. From the repository root on the Mac:

   ```bash
   cd ansible
   ansible-playbook local-runner.yml --check --diff -e runner_token=<token>
   ansible-playbook local-runner.yml -e runner_token=<token>
   ```

   Check mode validates prerequisites but never registers or installs the
   service (those steps are guarded). The play downloads the latest arm64
   release of `actions/runner`, extracts it under `/home/admin/actions-runner`
   (override the location or version with `-e actions_runner_dir=` /
   `-e actions_runner_version=X.Y.Z`), registers it with the label
   `quartus-vm`, and installs it as a systemd service so it survives VM
   restarts.
3. Confirm the runner shows as **Idle** under Settings -> Actions ->
   Runners, then dispatch `local-build.yml`. GitHub only registers a manual
   workflow after its file reaches the repository's default branch; before
   this change merges, the runner can be online but that workflow cannot yet
   be dispatched by filename.

### Upgrade and removal

Upgrading means unregistering and uninstalling the old service before deleting
the installation, then re-running the play with a fresh registration token:

```bash
ssh admin@quartus-vm.local
cd ~/actions-runner
sudo ./svc.sh stop
sudo ./svc.sh uninstall
./config.sh remove --token <fresh-removal-token>
rm -rf ~/actions-runner
```

Mint the removal token in the GitHub runner UI immediately before the command;
it expires like a registration token. Removing the runner only in the UI
leaves the VM's `.runner` marker behind, so also run `config.sh remove` locally
before re-provisioning.
`local-runner.yml` itself stays idempotent: an existing `.runner` marker and
service unit are left alone on rerun.
