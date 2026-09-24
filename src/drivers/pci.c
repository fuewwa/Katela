#include "pci.h"

#define PCI_ADDRESS_PORT 0xCF8
#define PCI_DATA_PORT    0xCFC

static inline void outl(unsigned short port, unsigned int value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned int inl(unsigned short port) {
    unsigned int value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static unsigned int make_address(const struct pci_device *dev, unsigned char offset) {
    return 0x80000000u
        | ((unsigned int)dev->bus << 16)
        | ((unsigned int)dev->slot << 11)
        | ((unsigned int)dev->func << 8)
        | (unsigned int)(offset & 0xFC);
}

unsigned int pci_read32(const struct pci_device *dev, unsigned char offset) {
    outl(PCI_ADDRESS_PORT, make_address(dev, offset));
    return inl(PCI_DATA_PORT);
}

void pci_write32(const struct pci_device *dev, unsigned char offset, unsigned int value) {
    outl(PCI_ADDRESS_PORT, make_address(dev, offset));
    outl(PCI_DATA_PORT, value);
}

static int class_matches(const struct pci_device *dev, unsigned char class_code, unsigned char subclass, unsigned char prog_if) {
    unsigned int reg = pci_read32(dev, PCI_REG_CLASS);

    return (unsigned char)(reg >> 24) == class_code
        && (unsigned char)(reg >> 16) == subclass
        && (unsigned char)(reg >> 8) == prog_if;
}

int pci_find_class(unsigned char class_code, unsigned char subclass, unsigned char prog_if, struct pci_device *dev) {
    unsigned int bus;
    unsigned int slot;
    unsigned int func;

    for (bus = 0; bus < 256; bus++) {
        for (slot = 0; slot < 32; slot++) {
            struct pci_device probe;
            unsigned int functions = 1;

            probe.bus = (unsigned char)bus;
            probe.slot = (unsigned char)slot;
            probe.func = 0;

            if ((pci_read32(&probe, PCI_REG_ID) & 0xFFFFu) == 0xFFFFu) {
                continue;
            }

            if ((pci_read32(&probe, PCI_REG_HEADER) >> 16) & 0x80u) {
                functions = 8;
            }

            for (func = 0; func < functions; func++) {
                probe.func = (unsigned char)func;

                if ((pci_read32(&probe, PCI_REG_ID) & 0xFFFFu) == 0xFFFFu) {
                    continue;
                }

                if (class_matches(&probe, class_code, subclass, prog_if)) {
                    *dev = probe;
                    return 0;
                }
            }
        }
    }

    return -1;
}

void pci_enable(const struct pci_device *dev, unsigned int bits) {
    unsigned int command = pci_read32(dev, PCI_REG_COMMAND) & 0xFFFFu;

    pci_write32(dev, PCI_REG_COMMAND, command | bits);
}
