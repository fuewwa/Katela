#ifndef PCI_H
#define PCI_H

#define PCI_REG_ID          0x00
#define PCI_REG_COMMAND     0x04
#define PCI_REG_CLASS       0x08
#define PCI_REG_HEADER      0x0C
#define PCI_REG_BAR0        0x10
#define PCI_REG_BAR1        0x14

#define PCI_COMMAND_MEMORY      0x0002u
#define PCI_COMMAND_MASTER      0x0004u
#define PCI_COMMAND_NO_INTX     0x0400u

struct pci_device {
    unsigned char bus;
    unsigned char slot;
    unsigned char func;
};

unsigned int pci_read32(const struct pci_device *dev, unsigned char offset);
void pci_write32(const struct pci_device *dev, unsigned char offset, unsigned int value);
int pci_find_class(unsigned char class_code, unsigned char subclass, unsigned char prog_if, struct pci_device *dev);
void pci_enable(const struct pci_device *dev, unsigned int bits);

#endif
