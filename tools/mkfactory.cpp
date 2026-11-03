#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "layout.h"

namespace {

const size_t SECTOR = 512;
const size_t KERNEL_OFFSET_SECTOR = 1;

std::vector<uint8_t> read_file(const char *path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "cannot read %s\n", path);
        std::exit(1);
    }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void write_file(const char *path, const std::vector<uint8_t> &data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        std::fprintf(stderr, "cannot write %s\n", path);
        std::exit(1);
    }
    out.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
}

void write_text(const char *path, const std::string &text) {
    write_file(path, std::vector<uint8_t>(text.begin(), text.end()));
}

std::string disk_config() {
    char buffer[1024];
    std::snprintf(buffer, sizeof(buffer),
        "set timeout=5\n"
        "set default=0\n"
        "menuentry \"Kernel\" {\n"
        "    multiboot (hd0)%d+%d\n"
        "    boot\n"
        "}\n"
        "menuentry \"Recovery\" {\n"
        "    multiboot (hd0)%d+%d recovery\n"
        "    module (hd0)0+%d factory\n"
        "    boot\n"
        "}\n",
        KERNEL_LBA, KERNEL_SECTORS, KERNEL_LBA, KERNEL_SECTORS, FACTORY_SECTORS);
    return buffer;
}

std::string embedded_config() {
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "configfile (hd0)%d+%d\n", BOOT_CFG_LBA, BOOT_CFG_SECTORS);
    return buffer;
}

void put64(std::vector<uint8_t> &data, size_t offset, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        data[offset + i] = static_cast<uint8_t>(value >> (8 * i));
    }
}

void put16(std::vector<uint8_t> &data, size_t offset, uint16_t value) {
    data[offset] = static_cast<uint8_t>(value);
    data[offset + 1] = static_cast<uint8_t>(value >> 8);
}

int build_image(char **argv) {
    std::vector<uint8_t> boot = read_file(argv[0]);
    std::vector<uint8_t> core = read_file(argv[1]);
    std::vector<uint8_t> kernel = read_file(argv[2]);
    std::vector<uint8_t> config = read_file(argv[3]);

    if (boot.size() != SECTOR) {
        std::fprintf(stderr, "boot image must be %zu bytes\n", SECTOR);
        return 1;
    }

    size_t core_sectors = (core.size() + SECTOR - 1) / SECTOR;
    if (KERNEL_OFFSET_SECTOR + core_sectors > static_cast<size_t>(BOOT_CFG_LBA)) {
        std::fprintf(stderr, "core image needs %zu sectors, only %d available\n", core_sectors, BOOT_CFG_LBA - 1);
        return 1;
    }
    if (config.size() > static_cast<size_t>(BOOT_CFG_SECTORS) * SECTOR) {
        std::fprintf(stderr, "boot config is too large\n");
        return 1;
    }
    if (kernel.size() > static_cast<size_t>(KERNEL_SECTORS) * SECTOR) {
        std::fprintf(stderr, "kernel is %zu bytes, limit is %d\n", kernel.size(), KERNEL_SECTORS * static_cast<int>(SECTOR));
        return 1;
    }

    core.resize(core_sectors * SECTOR, 0);

    put64(boot, 0x5C, KERNEL_OFFSET_SECTOR);
    boot[0x66] = 0x90;
    boot[0x67] = 0x90;

    const size_t first_block = SECTOR - 12;
    put64(core, first_block, KERNEL_OFFSET_SECTOR + 1);
    put16(core, first_block + 8, static_cast<uint16_t>(core_sectors - 1));
    put16(core, first_block + 10, 0x820);

    std::vector<uint8_t> image(static_cast<size_t>(FACTORY_SECTORS) * SECTOR, 0);
    std::memcpy(image.data(), boot.data(), SECTOR);
    std::memcpy(image.data() + KERNEL_OFFSET_SECTOR * SECTOR, core.data(), core.size());
    std::memcpy(image.data() + static_cast<size_t>(BOOT_CFG_LBA) * SECTOR, config.data(), config.size());
    std::memcpy(image.data() + static_cast<size_t>(KERNEL_LBA) * SECTOR, kernel.data(), kernel.size());

    write_file(argv[4], image);
    return 0;
}

}

int main(int argc, char **argv) {
    if (argc == 4 && std::strcmp(argv[1], "config") == 0) {
        write_text(argv[2], embedded_config());
        write_text(argv[3], disk_config());
        return 0;
    }
    if (argc == 7 && std::strcmp(argv[1], "image") == 0) {
        return build_image(argv + 2);
    }
    std::fprintf(stderr, "usage: mkfactory config EMBEDDED DISK | image BOOT CORE KERNEL DISKCFG OUT\n");
    return 1;
}
