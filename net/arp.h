#ifndef ARP_H
#define ARP_H

#include "addr.h"

#define ARP_PACKET_SIZE 28
#define ARP_HARDWARE_ETHERNET 1
#define ARP_PROTOCOL_IPV4 0x0800
#define ARP_REQUEST 1
#define ARP_REPLY 2

#define ARP_TABLE_SIZE 16
#define ARP_ENTRY_LIFETIME_MS 60000u

struct arp_packet {
	unsigned short operation;
	struct mac_addr sender_mac;
	struct ipv4_addr sender_ip;
	struct mac_addr target_mac;
	struct ipv4_addr target_ip;
};

struct arp_entry {
	struct ipv4_addr ip;
	struct mac_addr mac;
	unsigned int updated;
	unsigned char used;
};

struct arp_table {
	struct arp_entry entries[ARP_TABLE_SIZE];
};

int arp_parse(const void *data, unsigned int size, struct arp_packet *packet);
unsigned int arp_build(void *data, unsigned int capacity, const struct arp_packet *packet);
unsigned int arp_frame(void *frame, unsigned int capacity, const struct arp_packet *packet);

void arp_make_request(struct arp_packet *packet, const struct mac_addr *own_mac,
		const struct ipv4_addr *own_ip, const struct ipv4_addr *target_ip);
void arp_make_reply(struct arp_packet *packet, const struct mac_addr *own_mac,
		const struct ipv4_addr *own_ip, const struct arp_packet *request);

void arp_table_init(struct arp_table *table);
void arp_table_update(struct arp_table *table, const struct ipv4_addr *ip,
		const struct mac_addr *mac, unsigned int now);
int arp_table_lookup(struct arp_table *table, const struct ipv4_addr *ip,
		struct mac_addr *mac, unsigned int now);
void arp_table_remove(struct arp_table *table, const struct ipv4_addr *ip);
void arp_table_expire(struct arp_table *table, unsigned int now);

int arp_handle(struct arp_table *table, const struct arp_packet *packet,
		const struct mac_addr *own_mac, const struct ipv4_addr *own_ip,
		unsigned int now, struct arp_packet *reply);

#endif
