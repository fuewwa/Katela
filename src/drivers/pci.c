#include "pci.h"
#include "../../include/io.h"

#define PCI_ADDRESS_PORT 0xCF8
#define PCI_DATA_PORT    0xCFC

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

static int match_class(const struct pci_device *dev, const unsigned int *key) {
    unsigned int reg = pci_read32(dev, PCI_REG_CLASS);

    return (unsigned char)(reg >> 24) == key[0]
        && (unsigned char)(reg >> 16) == key[1]
        && (unsigned char)(reg >> 8) == key[2];
}

static int match_id(const struct pci_device *dev, const unsigned int *key) {
    unsigned int reg = pci_read32(dev, PCI_REG_ID);

    return (reg & 0xFFFFu) == key[0] && (reg >> 16) == key[1];
}

static int scan(int (*match)(const struct pci_device *, const unsigned int *), const unsigned int *key, struct pci_device *dev) {
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

                if (match(&probe, key)) {
                    *dev = probe;
                    return 0;
                }
            }
        }
    }

    return -1;
}

int pci_find_class(unsigned char class_code, unsigned char subclass, unsigned char prog_if, struct pci_device *dev) {
    unsigned int key[3];

    key[0] = class_code;
    key[1] = subclass;
    key[2] = prog_if;

    return scan(match_class, key, dev);
}

int pci_find_device(unsigned short vendor, unsigned short device, struct pci_device *dev) {
    unsigned int key[2];

    key[0] = vendor;
    key[1] = device;

    return scan(match_id, key, dev);
}

void pci_enable(const struct pci_device *dev, unsigned int bits) {
    unsigned int command = pci_read32(dev, PCI_REG_COMMAND) & 0xFFFFu;

    pci_write32(dev, PCI_REG_COMMAND, command | bits);
}
