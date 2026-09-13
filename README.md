# Katela

**Katela** is a real, standalone kernel written from scratch in C and x86 Assembly. Development began on **April 26, 2026**.

Katela is **not** a hobby os, **not** a toy os, **not** even an operating system, and it is **not** built as a learning exercise that stops at "hello world". It is engineered as a genuine kernel project: its own core, its own shell, and its own filesystem, all built with the intention of being a real, usable kernel — not a proof of concept and not a weekend experiment.

Katela does **not** follow UNIX or POSIX conventions. It does not aim for POSIX compliance, UNIX-like semantics, or compatibility with existing UNIX tooling. Katela follows its own design philosophy, its own system call conventions, its own shell behavior, and its own way of doing things from the ground up.

## Philosophy

Katela exists to explore what a kernel can look like when it isn't bound by UNIX/POSIX conventions or built purely as a hobby exercise. Every subsystem — from the shell to the filesystem to the drivers — is designed intentionally as part of a cohesive, independent kernel, not assembled as disconnected experiments.

## Requirements

To build and run Katela you need a cross-platform toolchain consisting of an assembler (NASM), a GCC-based cross/multilib compiler, GRUB tools for producing a bootable ISO, `xorriso` for ISO creation, and QEMU for emulation/testing (you also can try another VMs like virtualbox or wmware).

### Debian / Ubuntu

```bash
sudo apt install build-essential nasm grub-pc-bin xorriso qemu-system-x86 make gcc-multilib
```

### Arch Linux

```bash
sudo pacman -S base-devel nasm grub xorriso qemu-full make gcc mtools
```

> Note: on Arch, 32-bit multilib support requires enabling the `multilib` repository in `/etc/pacman.conf` first (uncomment the `[multilib]` section), then running `sudo pacman -Syu`.

### macOS

Install [Homebrew](https://brew.sh) first, then:

```bash
brew install nasm xorriso qemu make
brew install x86_64-elf-gcc x86_64-elf-binutils
```

> Note: macOS does not ship a native `grub-pc-bin` package. You'll need a cross-compiled GRUB toolchain (e.g. via `brew install i686-elf-grub` from a suitable tap, or by building GRUB from source) to produce a bootable ISO.

### Windows

The recommended approach is to use **WSL2** (Windows Subsystem for Linux) with a Debian or Ubuntu distribution, then follow the Debian/Ubuntu instructions above:

```powershell
wsl --install -d Ubuntu
```

Once inside WSL:

```bash
sudo apt install build-essential nasm grub-pc-bin xorriso qemu-system-x86 make gcc-multilib
```

Alternatively, native Windows builds are possible via **MSYS2**, installing equivalent packages (`nasm`, `mingw-w64-x86_64-toolchain`, `qemu`, `xorriso`, and a GRUB toolchain), though WSL2 is the more reliable path.

## Building

Build Katela:

```bash
make
```

Build and run Katela in QEMU:

```bash
make run
```

Clean all build artifacts:

```bash
make clean
```
## License

Katela is licensed under the GNU General Public License v3.0.
See the [LICENSE](LICENSE) file for the full license text.
