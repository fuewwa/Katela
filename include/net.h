#ifndef NET_H
#define NET_H

#include "../net/addr.h"

#define NET_OK 0
#define NET_ERR_NO_DEVICE -1
#define NET_ERR_NO_ROUTE -2
#define NET_ERR_TIMEOUT -3
#define NET_ERR_SEND -4
#define NET_ERR_NAME -5
#define NET_ERR_NXDOMAIN -6
#define NET_ERR_DNS -7

int net_init(void);
int net_ready(void);
int net_ping(const struct ipv4_addr *target, unsigned int timeout_ms, unsigned int *elapsed_ms);
int net_resolve(const char *name, struct ipv4_addr *address, unsigned int timeout_ms);
const struct mac_addr *net_mac(void);
const struct ipv4_addr *net_ip(void);
const struct ipv4_addr *net_gateway(void);
const struct ipv4_addr *net_dns(void);

#endif
