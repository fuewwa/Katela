#ifndef IPV4_H
#define IPV4_H

#include "addr.h"

#define IPV4_HEADER_SIZE 20
#define IPV4_PACKET_MAX 65535

#define IPV4_PROTOCOL_ICMP 1
#define IPV4_PROTOCOL_TCP 6
#define IPV4_PROTOCOL_UDP 17

#define IPV4_DEFAULT_TTL 64

#define IPV4_FLAG_DONT_FRAGMENT 0x4000u
#define IPV4_FLAG_MORE_FRAGMENTS 0x2000u
#define IPV4_FRAGMENT_OFFSET_MASK 0x1FFFu

#define IPV4_ERR_MALFORMED -1
#define IPV4_ERR_FRAGMENT -2

struct ipv4_header {
	unsigned char header_length;
	unsigned char tos;
	unsigned short total_length;
	unsigned short id;
	unsigned short flags_fragment;
	unsigned char ttl;
	unsigned char protocol;
	struct ipv4_addr source;
	struct ipv4_addr destination;
};

void ipv4_header_init(struct ipv4_header *header, const struct ipv4_addr *source,
		const struct ipv4_addr *destination, unsigned char protocol);
int ipv4_packet_parse(const void *packet, unsigned int size, struct ipv4_header *header,
		const unsigned char **payload, unsigned int *payload_size);
unsigned int ipv4_packet_build(void *packet, unsigned int capacity, const struct ipv4_header *header,
		const void *payload, unsigned int payload_size);
unsigned int ipv4_pseudo_sum(const struct ipv4_addr *source, const struct ipv4_addr *destination,
		unsigned char protocol, unsigned int length);
int ipv4_in_subnet(const struct ipv4_addr *a, const struct ipv4_addr *b, const struct ipv4_addr *mask);

#endif
