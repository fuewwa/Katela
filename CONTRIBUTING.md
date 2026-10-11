# Contributing to Katela

Thank you for your interest in contributing to Katela!

This file is the practical guide: how to build the project, where things live, what to watch out for when writing kernel code, and how to debug and test a change. The process itself (how a contribution travels from an idea to `main`, licensing, review) is described in `docs/process/DEVELOPMENT_PROCESS.md`.

## Before you start

Read these first. They are short, and they answer most questions:

- `README.md` explains what Katela is, how to install the toolchain, and the rules for using AI
- `docs/process/` directory with files that describes the contribution process, the license and etc
- `SECURITY.md` explains how security problems are handled

For a large change (a new subsystem, a change of the filesystem format, a new syscall convention) open an issue first and describe the idea.

## How to contribute

- Fork the repository
- Make your changes
- Submit a pull request

## Guidelines

- Keep the code simple and readable
- Follow the existing project structure
- Write clear commit messages
- Check README.md for notice about using AI

## Building and running

```bash
make clean
make
make run
```

| Command | What it does |
| --- | --- |
| `make` | builds `build/kernel.bin` and the bootable image `katela.iso` |
| `make run` | builds if needed, creates a 16 MB `disk.img`, boots in QEMU with an ATA (IDE) disk |
| `make run-nvme` | the same, but the disk is an NVMe device |
| `make clean` | removes `build/`, `katela.iso` and `disk.img` |

Things that are easy to trip over:

- The Makefile does not track dependencies. After you change a source file, run `make clean` before `make`, otherwise the old `build/kernel.bin` is reused.
- `make clean` also deletes `disk.img`, so every clean build starts on a blank disk. To test mounting a disk that was formatted earlier, copy `disk.img` aside before cleaning and boot it with `qemu-system-i386 -cdrom katela.iso -hda saved.img`.
- GRUB shows a menu when the image boots. Choose `Kernel`.
- The run targets use the ALSA audio backend for the PC speaker (the `beep` command). If the machine has no sound card, QEMU prints ALSA warnings but still runs. To use another backend, change the `-audiodev` option in the Makefile.

Headers of `crypto/` and `net/` live next to their sources, not in `include/`.

## Adding a source file

There is no wildcard rule. A new file needs two edits in the `Makefile`:

1. a compile line next to the others
2. the object file in the link list of `build/kernel.bin`

A C file is compiled like this:

```
gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c path/file.c -o $(BUILD)/file.o
```

Copy the whole flag set, including `-mno-sse -mno-mmx` (see below). If you introduce a new external tool, add its check to the `MISSING_TOOLS` block in `config/requirements.mk`.

## Writing kernel code

The kernel is freestanding C. A few rules follow from that, and each of them has caused a real bug:

- **No standard library.** Use `include/mem.h` for `memcpy`, `memset`, `strlen` and friends.
- **No 64-bit division or modulo.** The compiler turns `unsigned long long a / b` into a call to `__udivdi3`, which is not linked, so the build fails. Use 32-bit values or shifts.
- **No SSE, MMX or floating point.** The kernel never enables them. If the compiler emits one of these instructions anyway (newer GCC versions do this for plain struct copies), the CPU raises an invalid-opcode exception. That is why every compile line passes `-mno-sse -mno-mmx`. Do not use `float` or `double`.
- **Exceptions before the scheduler starts cannot be handled.** The IDT is loaded during scheduler start-up, after the filesystem is mounted. A fault in earlier code (disk probing, mounting) ends in a triple fault: the machine resets and QEMU shows the boot sequence again.
- **Hardware interrupts.** Register a handler with `irq_register(irq, handler, context)` from `include/irq.h`. Handlers run with interrupts disabled, so keep them short and never block. IRQ 0 (timer) and IRQ 2 (cascade) are reserved, and one IRQ line takes at most 4 handlers.
- **Hardware structures and DMA buffers** need `packed` and `aligned` attributes, see `docs/process/PROGRAMMING_LANGUAGE.md`.

Match the indentation of the file you are editing, follow `.editorconfig` in new files, and do not reformat lines you did not change.

## Debugging with QEMU

If the machine resets or QEMU exits right after you select `Kernel`, find out why before guessing:

```bash
qemu-system-i386 -cdrom katela.iso -hda disk.img -no-reboot -d int,cpu_reset,guest_errors -D qemu.log
```

`-no-reboot` stops QEMU at the first triple fault instead of rebooting. Then look at the end of the log and at the first exception:

```bash
tail -n 40 qemu.log
grep -n -m4 " v=0[0-9a-f] " qemu.log
```

`v=` is the exception vector and `IP=` is the address of the faulting instruction. Common vectors: `00` divide error, `06` invalid opcode, `08` double fault, `0d` general protection. A double fault right after another exception usually means the IDT is not loaded yet.

To see which instruction failed, disassemble around the address from the log:

```bash
objdump -d -M intel build/kernel.bin --start-address=0x<ip minus 0x20> --stop-address=0x<ip plus 0x20>
```

`-d int` writes a line for every interrupt, including every timer tick, so the log grows quickly once the scheduler runs. Stop QEMU as soon as you have reproduced the problem. `*.log` files are ignored by git.

For debug prints without a serial driver, send bytes to the QEMU debug console. Start QEMU with `-debugcon file:debug.log` and write to port `0xE9`:

```c
asm volatile("outb %0, $0xE9" : : "a"((unsigned char)'x'));
```

## Testing your change

There are no automated tests yet. The CI workflow only checks that the project builds. Run the result yourself:

- `make clean` and `make`, then boot it with `make run`
- if you touched the disk, the filesystem, PCI or interrupts, also boot with `make run-nvme`
- for disk and filesystem changes, boot twice: once on a blank disk and once more on the disk that the first boot formatted
- check the compiler output for new warnings
- state in the pull request what you tested and how

Timing bugs hide on a fast virtual disk. To test the disk code on a slow disk, throttle it:

```bash
qemu-system-i386 -cdrom katela.iso -drive file=disk.img,format=raw,if=ide,throttling.iops-total=40
```

## Commits and pull requests

- Use short messages in the past tense, like the existing history: `Added IRQ dispatch and handler registration`, `Fixed ATA i/o bug`
- One pull request does one thing
- If AI helped you write code, say so in the pull request description. Name the tool and the model, and confirm that you reviewed the code and take full responsibility, for example: `AI used: <tool and model>. I reviewed the code and take full responsibility for it.`

## Reporting bugs

Open an issue and include:

- the exact commands you ran (`make clean; make; make run`, or `make run-nvme`)
- the output of `gcc --version | head -1` and `qemu-system-i386 --version | head -1`, because some bugs depend on the compiler or the QEMU version
- what you expected and what happened, with a screenshot if the screen shows an error
- the last lines of `qemu.log` if the machine resets or QEMU exits
