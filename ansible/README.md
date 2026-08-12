# Quartus UTM VM provisioning

This directory prepares the existing Debian 13 arm64 VM at `192.168.64.3`
for local Amstrad MiSTer builds. It restores `renaud`'s installer-created
groups, mounts UTM's Rosetta runtime, registers the x86_64 ELF handler,
enables Debian amd64 multiarch, and installs the native and amd64 runtime
dependencies used by Quartus and the repository's Verilator tests.

It deliberately does **not** download, copy, or accept the license terms for
Intel's proprietary installer. No credentials or installer files belong in
this repository.

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
skipped in check mode. The validation play executes Debian's amd64 `/usr/bin/hello`
through Rosetta, then reports the Quartus version if Quartus is installed.

The inventory connects as root using the already-configured SSH key. It does
not configure passwords or copy private/public keys. If the VM address changes,
edit `inventory.yml` or override it temporarily:

```bash
ansible-playbook -i 'quartus-vm,' \
  -e ansible_host=192.168.64.3 -e ansible_user=root site.yml
```

## Manual Intel installer handoff

1. As a human, sign in to Intel's FPGA Software Download Center and download
   the Linux **Quartus Prime Lite 17.0** base installer, the **17.0.2** update,
   and **Cyclone V** device support. Review and accept Intel's terms yourself.
2. Transfer those files to a temporary directory in the VM. Do not put them in
   this repository. The Cyclone V `.qdz` must be beside the base installer.
3. SSH into the VM as `renaud`, make the installers executable, and install the
   base release into the provisioned user-owned directory:

   ```bash
   ssh renaud@192.168.64.3
   cd /path/to/temporary/installer-directory
   chmod +x QuartusLiteSetup-17.0.0.595-linux.run
   export QUARTUS_CPUID_BYPASS=1
   ./QuartusLiteSetup-17.0.0.595-linux.run \
     --mode unattended --installdir /home/renaud/intelFPGA_lite/17.0
   ```

   `--mode unattended` is only appropriate after you personally review and
   accept the download's terms. If Intel's installer requires an explicit EULA
   option, use its `--help` output rather than copying a flag from a different
   Quartus release.
4. Apply Intel's 17.0.2 update to the same installation directory, following
   the update installer's own `--help`/GUI. Installer filenames and switches
   have changed across archive revisions, so the playbook does not guess them.
5. Remove the temporary installer files when you no longer need them. They are
   proprietary and can always be downloaded again from your Intel account.
6. From the Mac, apply the Rosetta compatibility patch to Quartus's environment
   script, then validate the result:

   ```bash
   cd ansible
   ansible-playbook post-install.yml --check --diff
   ansible-playbook post-install.yml
   ansible-playbook post-install.yml --check --diff
   ansible-playbook validate.yml
   ```

   `post-install.yml` recognizes the two relevant sections of Quartus 17's
   `quartus/adm/qenv.sh` before changing anything, creates a one-time
   `qenv.sh.pre-rosetta` backup, selects the 64-bit tools on an aarch64 guest,
   and disables only the host SSE-probe section that cannot work through
   Rosetta. It then runs `quartus_sh --version` as `renaud` and requires
   version 17.0.2. Re-running the play makes no further edit.

Rosetta only translates 64-bit Intel Linux executables. Quartus synthesis uses
the 64-bit tools; legacy 32-bit utilities and USB/JTAG tooling are outside this
VM's supported build-only scope.

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
