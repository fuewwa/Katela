#ifndef RTL8139_H
#define RTL8139_H

#include "../../net/addr.h"

#define RTL8139_OK 0
#define RTL8139_ERR_NOT_FOUND -1
#define RTL8139_ERR_BAR -2
#define RTL8139_ERR_RESET -3
#define RTL8139_ERR_IRQ -4

int rtl8139_init(void);
int rtl8139_send(const void *frame, unsigned int size);
int rtl8139_receive(void *buffer, unsigned int capacity, unsigned int timeout_ms);
const struct mac_addr *rtl8139_mac(void);
int rtl8139_ready(void);

#endif
