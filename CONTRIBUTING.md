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

Katela builds for two architectures. `ARCH=x86` is the default. `ARCH=x86_64` builds the 64-bit version. Put `ARCH=x86_64` after `make` in every command below, for example `make ARCH=x86_64 run`.

| Command | What it does |
| --- | --- |
| `make` | builds `build/kernel.bin` and the bootable image `katela.iso` |
| `make run` | builds if needed, creates a 16 MB `disk.img`, boots in QEMU with an ATA (IDE) disk |
| `make run-nvme` | the same, but the disk is an NVMe device |
| `make ARCH=x86_64` | builds `build/x86_64/kernel.bin` and the bootable image `katela-x86_64.iso` |
| `make ARCH=x86_64 run` | the same as `make run`, but boots the x86_64 version in `qemu-system-x86_64` |
| `make ARCH=x86_64 run-nvme` | the same as `make run-nvme`, for the x86_64 version |
| `make clean` | removes `build/` (all architectures), both ISO images and `disk.img` |

Things that are easy to trip over:

- The Makefile does not track dependencies. After you change a source file, run `make clean` before `make`, otherwise the old `build/kernel.bin` is reused.
- `make clean` also deletes `disk.img`, so every clean build starts on a blank disk. To test mounting a disk that was formatted earlier, copy `disk.img` aside before cleaning and boot it with `qemu-system-i386 -cdrom katela.iso -hda saved.img` (`qemu-system-x86_64` and `katela-x86_64.iso` for the x86_64 version). The disk format is the same on both architectures.
- GRUB shows a menu when the image boots. Choose `Kernel`. The console then runs on a 1024x768 framebuffer. The window size of QEMU shows which console is active: 1024x768 is the framebuffer, 720x400 is the VGA text fallback.
- The run targets use the ALSA audio backend for the PC speaker (the `beep` command). If the machine has no sound card, QEMU prints ALSA warnings but still runs. To use another backend, change the `-audiodev` option in `config/pc.mk`.

Headers of `crypto/` and `net/` live next to their sources, not in `include/`.

## Adding a source file

There is no wildcard rule. A new file needs two edits:

1. a compile line next to the others
2. the object file in the link list

For a file in `src/`, `crypto/` or `net/` both edits are in the `Makefile` (the link list is the `$(LD)` command of `$(KERNEL)`). For a file in `arch/<name>/` both edits are in `arch/<name>/arch.mk` (the compile line in `ARCH_BUILD`, the object in `ARCH_OBJECTS`). Object file names must be unique across all directories.

On x86 a C file is compiled like this:

```
gcc -m32 -mno-sse -mno-mmx -ffreestanding -Iinclude -Iarch/x86 -nostdlib -fno-stack-protector -c path/file.c -o $(BUILD)/file.o
```

On x86_64 it is compiled like this:

```
gcc -m64 -mno-sse -mno-mmx -mno-red-zone -fno-pic -fno-pie -fno-asynchronous-unwind-tables -ffreestanding -Iinclude -Iarch/x86_64 -Iarch/x86 -nostdlib -fno-stack-protector -c path/file.c -o $(BUILD)/file.o
```

In the Makefile use `$(CC) $(CFLAGS)` instead of typing the flags. When you compile by hand, copy the whole flag set, including `-mno-sse -mno-mmx` (see below). If you introduce a new external tool, add its check to the `MISSING_TOOLS` block in `config/requirements.mk`.

## Where the code lives

Code that does not depend on the processor mode lives in `src/`, `include/`, `crypto/` and `net/`. Code that does depend on it lives in `arch/<name>/`, together with its headers:

- `arch/x86/` holds the 32-bit boot code, GDT, IDT, interrupt stubs, user mode entry, `linker.ld` and `arch.mk`. It also holds the PC drivers that both architectures use: PIC, PIT, VGA text, PS/2 keyboard, PC speaker, ATA, port I/O (`io.h`), recovery mode, the installer and power control.
- `arch/x86_64/` holds the 64-bit versions of the boot code (it enters long mode and builds the page tables), GDT, TSS, IDT, interrupt stubs, user mode entry, `linker.ld` and `arch.mk`. Its `arch.mk` compiles the shared PC drivers from `arch/x86/`.

The common code includes `arch.h`, and the compiler finds the right one through `-Iarch/...` (`ARCH_INCLUDES` in `arch.mk`). `arch.h` defines `IRQ_COUNT`, `ARCH_STACK_SIZE`, `irq_save`, `irq_restore`, `irq_enable`, `cpu_idle`, `cpu_stop`, `memory_barrier`, `context_yield` and `user_syscall`, and declares `cpu_init`, `interrupts_init`, `timer_init` and `context_init`. Files in `arch/x86/` that both architectures compile must not include `arch.h`, `gdt.h` or `idt.h` directly, because a quoted include finds the header of `arch/x86/` first. They get what they need through `include/irq.h` and the other common headers.

The rules for the common code are:

- No `asm` statements, no port I/O and no fixed hardware addresses in `src/`. Put them in `arch/` and call them through `arch.h`.
- Use `unsigned long` for anything that holds an address or a stack pointer. It is 32 bits wide on x86 and 64 bits wide on x86_64. `int` and `unsigned int` are 32 bits wide on both.
- A pointer stored in a 32-bit field (a DMA address for NVMe or the RTL8139) must point below 4 GB. The kernel image, its heap and its stacks are there, and x86_64 maps exactly the first 4 GB.

## Writing kernel code

The kernel is freestanding C. A few rules follow from that, and each of them has caused a real bug:

- **No standard library.** Use `include/mem.h` for `memcpy`, `memset`, `strlen` and friends.
- **No 64-bit division or modulo.** The compiler turns `unsigned long long a / b` into a call to `__udivdi3`, which is not linked, so the build fails. Use 32-bit values or shifts.
- **No SSE, MMX or floating point.** The kernel never enables them. If the compiler emits one of these instructions anyway (newer GCC versions do this for plain struct copies), the CPU raises an invalid-opcode exception. That is why every compile line passes `-mno-sse -mno-mmx`. Do not use `float` or `double`.
- **No red zone on x86_64.** Interrupt handlers push onto the stack of the code they interrupt, and the red zone of the 64-bit ABI would be overwritten. That is why x86_64 compile lines pass `-mno-red-zone`. Never remove it.
- **Faults on x86_64.** After the scheduler has started, a general protection fault, a page fault or a double fault prints `KERNEL PANIC` with the name of the fault and stops. Only the first 4 GB are mapped, so touching an address above it is a page fault. On x86 the same mistakes are not caught (there is no paging), and a double fault ends in a reset.
- **Exceptions before the scheduler starts cannot be handled.** The IDT is loaded during scheduler start-up, after the filesystem is mounted. A fault in earlier code (disk probing, mounting) ends in a triple fault: the machine resets and QEMU shows the boot sequence again.
- **Hardware interrupts.** Register a handler with `irq_register(irq, handler, context)` from `include/irq.h`. Handlers run with interrupts disabled, so keep them short and never block. IRQ 0 (timer) and IRQ 2 (cascade) are reserved, and one IRQ line takes at most 4 handlers.
- **Hardware structures and DMA buffers** need `packed` and `aligned` attributes, see `docs/process/PROGRAMMING_LANGUAGE.md`.

Match the indentation of the file you are editing, follow `.editorconfig` in new files, and do not reformat lines you did not change.

## Console

The kernel asks GRUB for a 1024x768 32-bit linear framebuffer through the Multiboot header in `arch/x86/boot.asm` and `arch/x86_64/boot.asm`. `console_init()` reads the framebuffer description from the Multiboot information. If GRUB did not set up a usable framebuffer, the console stays in VGA text mode. Both modes are used through `src/drivers/console.h`:

- `print`, `backspace` and `clear` for normal output
- `console_cols`, `console_rows`, `console_put` and `console_hide_cursor` for code that places characters itself, like recovery mode

Do not write to `0xB8000` or to the framebuffer outside of `src/drivers/vga.c` and `src/drivers/framebuffer.c`. The framebuffer driver keeps a copy of every character cell and draws 8x16 glyphs from `src/drivers/font.c`. Only printable ASCII is drawn, anything else is drawn as `?`. To regenerate the font run `python3 tools/mkfont.py unscii-16.hex src/drivers/font.c`.

The video modules of GRUB (`all_video`) are needed for the mode switch. They are listed in `GRUB_MODULES` in `config/pc.mk`, for the factory image, and loaded by `insmod` in `iso/boot/grub/grub.cfg`, for the ISO.

## Debugging with QEMU

If the machine resets or QEMU exits right after you select `Kernel`, find out why before guessing:

```bash
qemu-system-i386 -cdrom katela.iso -hda disk.img -no-reboot -d int,cpu_reset,guest_errors -D qemu.log
```

(use `qemu-system-x86_64` and `katela-x86_64.iso` for the x86_64 version)

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

The x86_64 kernel is a 32-bit ELF container with 64-bit code inside. `objdump` shows nonsense unless you force the machine type:

```bash
objdump -d -M intel -m i386:x86-64 build/x86_64/kernel.bin --start-address=0x<ip minus 0x20> --stop-address=0x<ip plus 0x20>
```

`-d int` writes a line for every interrupt, including every timer tick, so the log grows quickly once the scheduler runs. Stop QEMU as soon as you have reproduced the problem. `*.log` files are ignored by git.

For debug prints without a serial driver, send bytes to the QEMU debug console. Start QEMU with `-debugcon file:debug.log` and write to port `0xE9`:

```c
asm volatile("outb %0, $0xE9" : : "a"((unsigned char)'x'));
```

## Testing your change

There are no automated tests yet. The CI workflow only checks that the project builds. Run the result yourself:

- `make clean` and `make`, then boot it with `make run`
- if you touched anything in `arch/`, `include/`, `src/` or the Makefile, also build and boot the other architecture: `make clean`, `make ARCH=x86_64` and `make ARCH=x86_64 run`, and boot the x86 version again
- if you touched the disk, the filesystem, PCI or interrupts, also boot with `make run-nvme`
- if you touched the console, boot once with the framebuffer and once in the VGA text fallback (a kernel built without the video request in `arch/x86/boot.asm`), and open the recovery menu in both
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

- the exact commands you ran (`make clean; make; make run`, `make run-nvme` or `make ARCH=x86_64 run`)
- the architecture you built for
- the output of `gcc --version | head -1` and `qemu-system-i386 --version | head -1` (or `qemu-system-x86_64 --version | head -1`), because some bugs depend on the compiler or the QEMU version
- what you expected and what happened, with a screenshot if the screen shows an error
- the last lines of `qemu.log` if the machine resets or QEMU exits
