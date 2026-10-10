#ifndef ETHERNET_H
#define ETHERNET_H

#include "addr.h"

#define ETHERNET_HEADER_SIZE 14
#define ETHERNET_MTU 1500
#define ETHERNET_FRAME_MIN 60
#define ETHERNET_FRAME_MAX (ETHERNET_HEADER_SIZE + ETHERNET_MTU)

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IPV6 0x86DD

struct ethernet_header {
	struct mac_addr destination;
	struct mac_addr source;
	unsigned short type;
};

int ethernet_parse(const void *frame, unsigned int size, struct ethernet_header *header,
		const unsigned char **payload, unsigned int *payload_size);
unsigned int ethernet_build(void *frame, unsigned int capacity, const struct ethernet_header *header,
		const void *payload, unsigned int payload_size);
int ethernet_for_us(const struct ethernet_header *header, const struct mac_addr *own);

#endif
