#include "../../include/recovery.h"
#include "io.h"

struct table_header {
	char signature[4];
	unsigned int length;
	unsigned char revision;
	unsigned char checksum;
	char oem[6];
	char oem_table[8];
	unsigned int oem_revision;
	unsigned int creator;
	unsigned int creator_revision;
} __attribute__((packed));

struct rsdp {
	char signature[8];
	unsigned char checksum;
	char oem[6];
	unsigned char revision;
	unsigned int rsdt;
} __attribute__((packed));

static int same(const char *a, const char *b, unsigned int n) {
	for (unsigned int i = 0; i < n; i++) {
		if (a[i] != b[i]) {
			return 0;
		}
	}
	return 1;
}

static int checksum_ok(const void *data, unsigned int length) {
	const unsigned char *bytes = data;
	unsigned char sum = 0;

	for (unsigned int i = 0; i < length; i++) {
		sum = (unsigned char)(sum + bytes[i]);
	}
	return sum == 0;
}

static const struct rsdp *scan_rsdp(unsigned int start, unsigned int end) {
	for (unsigned int addr = start; addr + sizeof(struct rsdp) <= end; addr += 16) {
		const struct rsdp *candidate = (const struct rsdp *)(unsigned long)addr;

		if (same(candidate->signature, "RSD PTR ", 8) && checksum_ok(candidate, 20)) {
			return candidate;
		}
	}
	return 0;
}

static const struct rsdp *find_rsdp(void) {
	unsigned int ebda = (unsigned int)(*(const unsigned short *)0x40E) << 4;

	if (ebda >= 0x80000 && ebda < 0xA0000) {
		const struct rsdp *found = scan_rsdp(ebda, ebda + 1024);

		if (found) {
			return found;
		}
	}
	return scan_rsdp(0xE0000, 0x100000);
}

static const struct table_header *find_table(const struct rsdp *rsdp, const char *signature) {
	const struct table_header *rsdt = (const struct table_header *)(unsigned long)rsdp->rsdt;

	if (!same(rsdt->signature, "RSDT", 4)) {
		return 0;
	}

	unsigned int count = (rsdt->length - sizeof(struct table_header)) / 4;
	const unsigned int *entries = (const unsigned int *)((const char *)rsdt + sizeof(struct table_header));

	for (unsigned int i = 0; i < count; i++) {
		const struct table_header *table = (const struct table_header *)(unsigned long)entries[i];

		if (same(table->signature, signature, 4)) {
			return table;
		}
	}
	return 0;
}

static unsigned int read32(const char *base, unsigned int offset) {
	return *(const unsigned int *)(base + offset);
}

static int find_sleep_types(const struct table_header *dsdt, unsigned int *type_a, unsigned int *type_b) {
	const unsigned char *begin = (const unsigned char *)dsdt + sizeof(struct table_header);
	const unsigned char *end = (const unsigned char *)dsdt + dsdt->length;

	for (const unsigned char *p = begin; p + 8 < end; p++) {
		if (!same((const char *)p, "_S5_", 4)) {
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
		*type_a = *q++;
		if (*q == 0x0A) {
			q++;
		}
		*type_b = *q;
		return 1;
	}
	return 0;
}

static void acpi_power_off(void) {
	const struct rsdp *rsdp = find_rsdp();

	if (!rsdp) {
		return;
	}

	const struct table_header *fadt = find_table(rsdp, "FACP");

	if (!fadt) {
		return;
	}

	const char *base = (const char *)fadt;
	const struct table_header *dsdt = (const struct table_header *)(unsigned long)read32(base, 40);
	unsigned int smi_command = read32(base, 48);
	unsigned char acpi_enable = (unsigned char)base[52];
	unsigned int pm1a = read32(base, 64);
	unsigned int pm1b = read32(base, 68);

	if (!dsdt || pm1a == 0 || !same(dsdt->signature, "DSDT", 4)) {
		return;
	}

	unsigned int type_a = 0;
	unsigned int type_b = 0;

	if (!find_sleep_types(dsdt, &type_a, &type_b)) {
		return;
	}

	if (!(inw((unsigned short)pm1a) & 1) && smi_command != 0 && acpi_enable != 0) {
		outb((unsigned short)smi_command, acpi_enable);
		for (unsigned int i = 0; i < 300000; i++) {
			if (inw((unsigned short)pm1a) & 1) {
				break;
			}
			io_wait();
		}
	}

	outw((unsigned short)pm1a, (unsigned short)((type_a << 10) | (1 << 13)));
	if (pm1b != 0) {
		outw((unsigned short)pm1b, (unsigned short)((type_b << 10) | (1 << 13)));
	}
	for (unsigned int i = 0; i < 1000000; i++) {
		io_wait();
	}
}

void recovery_power_off(void) {
	asm volatile("cli");
	acpi_power_off();
	outw(0x604, 0x2000);
	outw(0xB004, 0x2000);
	outw(0x4004, 0x3400);
	while (1) {
		asm volatile("hlt");
	}
}

void recovery_reboot(void) {
	asm volatile("cli");

	for (unsigned int i = 0; i < 0x10000; i++) {
		if (!(inb(0x64) & 2)) {
			break;
		}
	}
	outb(0x64, 0xFE);
	for (unsigned int i = 0; i < 100000; i++) {
		io_wait();
	}

	outb(0xCF9, 0x02);
	outb(0xCF9, 0x06);
	for (unsigned int i = 0; i < 100000; i++) {
		io_wait();
	}

	struct {
		unsigned short limit;
		unsigned long base;
	} __attribute__((packed)) empty = {0, 0};

	asm volatile("lidt %0\n\tint $3" : : "m"(empty));

	while (1) {
		asm volatile("hlt");
	}
}
