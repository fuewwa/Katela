#ifndef UDP_H
#define UDP_H

#include "addr.h"

#define UDP_HEADER_SIZE 8

#define UDP_ERR_MALFORMED -1
#define UDP_ERR_CHECKSUM -2

struct udp_header {
	unsigned short source_port;
	unsigned short destination_port;
	unsigned short length;
};

int udp_parse(const void *data, unsigned int size, const struct ipv4_addr *source,
		const struct ipv4_addr *destination, struct udp_header *header,
		const unsigned char **payload, unsigned int *payload_size);
unsigned int udp_build(void *data, unsigned int capacity, const struct ipv4_addr *source,
		const struct ipv4_addr *destination, unsigned short source_port,
		unsigned short destination_port, const void *payload, unsigned int payload_size);

#endif
