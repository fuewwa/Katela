import re
import struct
import sys

SECTOR = 512


def load_layout(path):
    values = {}
    for line in open(path).read().splitlines():
        match = re.match(r"#define\s+(\w+)\s+(\d+)", line)
        if match:
            values[match.group(1)] = int(match.group(2))
    return values


def disk_config(layout):
    return (
        "set timeout=5\n"
        "set default=0\n"
        'menuentry "Kernel" {\n'
        "    multiboot (hd0)%d+%d\n"
        "    boot\n"
        "}\n"
        'menuentry "Recovery" {\n'
        "    multiboot (hd0)%d+%d recovery\n"
        "    module (hd0)0+%d factory\n"
        "    boot\n"
        "}\n"
    ) % (
        layout["KERNEL_LBA"],
        layout["KERNEL_SECTORS"],
        layout["KERNEL_LBA"],
        layout["KERNEL_SECTORS"],
        layout["FACTORY_SECTORS"],
    )


def embedded_config(layout):
    return "configfile (hd0)%d+%d\n" % (layout["BOOT_CFG_LBA"], layout["BOOT_CFG_SECTORS"])


def build_image(layout, boot_path, core_path, kernel_path, config_path, out_path):
    boot = bytearray(open(boot_path, "rb").read())
    core = bytearray(open(core_path, "rb").read())
    kernel = open(kernel_path, "rb").read()
    config = open(config_path, "rb").read()

    cfg_lba = layout["BOOT_CFG_LBA"]
    cfg_sectors = layout["BOOT_CFG_SECTORS"]
    kernel_lba = layout["KERNEL_LBA"]
    kernel_sectors = layout["KERNEL_SECTORS"]
    total = layout["FACTORY_SECTORS"]

    if len(boot) != SECTOR:
        sys.exit("boot image must be %d bytes" % SECTOR)

    core_sectors = (len(core) + SECTOR - 1) // SECTOR
    if 1 + core_sectors > cfg_lba:
        sys.exit("core image needs %d sectors, only %d available" % (core_sectors, cfg_lba - 1))
    if len(config) > cfg_sectors * SECTOR:
        sys.exit("boot config is too large")
    if len(kernel) > kernel_sectors * SECTOR:
        sys.exit("kernel is %d bytes, limit is %d" % (len(kernel), kernel_sectors * SECTOR))

    core.extend(b"\0" * (core_sectors * SECTOR - len(core)))

    boot[0x5C:0x64] = struct.pack("<Q", 1)
    boot[0x66] = 0x90
    boot[0x67] = 0x90

    first_block = SECTOR - 12
    core[first_block:first_block + 12] = struct.pack("<QHH", 2, core_sectors - 1, 0x820)

    image = bytearray(total * SECTOR)
    image[0:SECTOR] = boot
    image[SECTOR:SECTOR + len(core)] = core
    image[cfg_lba * SECTOR:cfg_lba * SECTOR + len(config)] = config
    image[kernel_lba * SECTOR:kernel_lba * SECTOR + len(kernel)] = kernel

    open(out_path, "wb").write(image)


def main():
    if len(sys.argv) == 5 and sys.argv[1] == "config":
        layout = load_layout(sys.argv[2])
        open(sys.argv[3], "w").write(embedded_config(layout))
        open(sys.argv[4], "w").write(disk_config(layout))
        return
    if len(sys.argv) == 8 and sys.argv[1] == "image":
        build_image(load_layout(sys.argv[2]), *sys.argv[3:8])
        return
    sys.exit("usage: mkfactory.py config LAYOUT EMBEDDED DISK | image LAYOUT BOOT CORE KERNEL DISKCFG OUT")


main()
