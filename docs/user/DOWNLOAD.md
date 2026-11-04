## Selecting an ISO

First, you need to decide exactly what you're going to use and install, there are several options:

* Distribution - This distribution was created by other developers; choose only verified distributions. The distributors bear full responsibility (if this error occurs only in their distribution).

* Katela From Scratch - It's not recommended for everyday use, since you'll end up with just a base on which distributions will be built later, but you can do it if you want to.

Once you've chosen the method that works best for you, go to the website or GitHub page and download the latest version, or build it yourself. If the distribution isn't open source, we don't recommend downloading it.

Once you've found the ISO file you need, you'll need a USB flash drive and an ISO extraction program. Insert the USB flash drive into your computer, open the ISO file in an ISO extraction program, and then start the download.

> [!WARNING]
> The download process varies for each extractor, so to keep this page from getting too long, please refer to the wiki for the extractor you've chosen.

## Boot

The system boots using the 16-bit bootloader (GRUB), after which the kernel switches to 32-bit mode and begins initialization: the file system, GDT, Task Scheduler, audio, and so on. The system then mounts the disk and only then loads into the shell.

> [!IMPORTANT]
> Once again, I'll say it: things may vary from one distribution to another, so here I'm talking specifically about Kernel itself
