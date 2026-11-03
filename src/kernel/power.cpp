#include "recovery.hpp"
#include "../../include/io.h"

namespace {

struct TableHeader {
    char signature[4];
    unsigned length;
    unsigned char revision;
    unsigned char checksum;
    char oem[6];
    char oem_table[8];
    unsigned oem_revision;
    unsigned creator;
    unsigned creator_revision;
} __attribute__((packed));

struct Rsdp {
    char signature[8];
    unsigned char checksum;
    char oem[6];
    unsigned char revision;
    unsigned rsdt;
} __attribute__((packed));

bool same(const char *a, const char *b, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

bool checksum_ok(const void *data, unsigned length) {
    const unsigned char *bytes = static_cast<const unsigned char *>(data);
    unsigned char sum = 0;
    for (unsigned i = 0; i < length; i++) {
        sum = static_cast<unsigned char>(sum + bytes[i]);
    }
    return sum == 0;
}

const Rsdp *scan_rsdp(unsigned start, unsigned end) {
    for (unsigned addr = start; addr + sizeof(Rsdp) <= end; addr += 16) {
        const Rsdp *candidate = reinterpret_cast<const Rsdp *>(addr);
        if (same(candidate->signature, "RSD PTR ", 8) && checksum_ok(candidate, 20)) {
            return candidate;
        }
    }
    return 0;
}

const Rsdp *find_rsdp() {
    unsigned ebda = static_cast<unsigned>(*reinterpret_cast<const unsigned short *>(0x40E)) << 4;
    if (ebda >= 0x80000 && ebda < 0xA0000) {
        const Rsdp *found = scan_rsdp(ebda, ebda + 1024);
        if (found) {
            return found;
        }
    }
    return scan_rsdp(0xE0000, 0x100000);
}

const TableHeader *find_table(const Rsdp *rsdp, const char *signature) {
    const TableHeader *rsdt = reinterpret_cast<const TableHeader *>(rsdp->rsdt);
    if (!same(rsdt->signature, "RSDT", 4)) {
        return 0;
    }
    unsigned count = (rsdt->length - sizeof(TableHeader)) / 4;
    const unsigned *entries = reinterpret_cast<const unsigned *>(reinterpret_cast<const char *>(rsdt) + sizeof(TableHeader));
    for (unsigned i = 0; i < count; i++) {
        const TableHeader *table = reinterpret_cast<const TableHeader *>(entries[i]);
        if (same(table->signature, signature, 4)) {
            return table;
        }
    }
    return 0;
}

unsigned read32(const char *base, unsigned offset) {
    return *reinterpret_cast<const unsigned *>(base + offset);
}

bool find_sleep_types(const TableHeader *dsdt, unsigned &type_a, unsigned &type_b) {
    const unsigned char *begin = reinterpret_cast<const unsigned char *>(dsdt) + sizeof(TableHeader);
    const unsigned char *end = reinterpret_cast<const unsigned char *>(dsdt) + dsdt->length;

    for (const unsigned char *p = begin; p + 8 < end; p++) {
        if (!same(reinterpret_cast<const char *>(p), "_S5_", 4)) {
            continue;
        }
        const unsigned char *q = p + 4;
        if (*q != 0x12) {
            continue;
        }
        q++;
        q += ((*q >> 6) & 3) + 1;
        q++;
        if (*q == 0x0A) {
            q++;
        }
        type_a = *q++;
        if (*q == 0x0A) {
            q++;
        }
        type_b = *q;
        return true;
    }
    return false;
}

void acpi_power_off() {
    const Rsdp *rsdp = find_rsdp();
    if (!rsdp) {
        return;
    }

    const TableHeader *fadt = find_table(rsdp, "FACP");
    if (!fadt) {
        return;
    }

    const char *base = reinterpret_cast<const char *>(fadt);
    const TableHeader *dsdt = reinterpret_cast<const TableHeader *>(read32(base, 40));
    unsigned smi_command = read32(base, 48);
    unsigned char acpi_enable = static_cast<unsigned char>(base[52]);
    unsigned pm1a = read32(base, 64);
    unsigned pm1b = read32(base, 68);

    if (!dsdt || pm1a == 0 || !same(dsdt->signature, "DSDT", 4)) {
        return;
    }

    unsigned type_a = 0;
    unsigned type_b = 0;
    if (!find_sleep_types(dsdt, type_a, type_b)) {
        return;
    }

    if (!(inw(static_cast<unsigned short>(pm1a)) & 1) && smi_command != 0 && acpi_enable != 0) {
        outb(static_cast<unsigned short>(smi_command), acpi_enable);
        for (unsigned i = 0; i < 300000; i++) {
            if (inw(static_cast<unsigned short>(pm1a)) & 1) {
                break;
            }
            io_wait();
        }
    }

    outw(static_cast<unsigned short>(pm1a), static_cast<unsigned short>((type_a << 10) | (1 << 13)));
    if (pm1b != 0) {
        outw(static_cast<unsigned short>(pm1b), static_cast<unsigned short>((type_b << 10) | (1 << 13)));
    }
    for (unsigned i = 0; i < 1000000; i++) {
        io_wait();
    }
}

}

namespace recovery {

void power_off() {
    asm volatile("cli");
    acpi_power_off();
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
    while (true) {
        asm volatile("hlt");
    }
}

void reboot() {
    asm volatile("cli");

    for (unsigned i = 0; i < 0x10000; i++) {
        if (!(inb(0x64) & 2)) {
            break;
        }
    }
    outb(0x64, 0xFE);
    for (unsigned i = 0; i < 100000; i++) {
        io_wait();
    }

    outb(0xCF9, 0x02);
    outb(0xCF9, 0x06);
    for (unsigned i = 0; i < 100000; i++) {
        io_wait();
    }

    struct {
        unsigned short limit;
        unsigned base;
    } __attribute__((packed)) empty = {0, 0};
    asm volatile("lidt %0\n\tint $3" : : "m"(empty));

    while (true) {
        asm volatile("hlt");
    }
}

}
