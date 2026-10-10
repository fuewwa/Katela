#include "arp.h"
#include "ethernet.h"
#include "byteorder.h"
#include "mem.h"

int arp_parse(const void *data, unsigned int size, struct arp_packet *packet)
{
	const unsigned char *bytes = (const unsigned char *)data;
	unsigned short operation;

	if (size < ARP_PACKET_SIZE) {
		return -1;
	}

	if (be16_read(bytes) != ARP_HARDWARE_ETHERNET
		|| be16_read(bytes + 2) != ARP_PROTOCOL_IPV4
		|| bytes[4] != MAC_SIZE
		|| bytes[5] != IPV4_SIZE) {
		return -1;
	}

	operation = be16_read(bytes + 6);
	if (operation != ARP_REQUEST && operation != ARP_REPLY) {
		return -1;
	}

	packet->operation = operation;
	memcpy(packet->sender_mac.bytes, bytes + 8, MAC_SIZE);
	memcpy(packet->sender_ip.bytes, bytes + 14, IPV4_SIZE);
	memcpy(packet->target_mac.bytes, bytes + 18, MAC_SIZE);
	memcpy(packet->target_ip.bytes, bytes + 24, IPV4_SIZE);

	return 0;
}

unsigned int arp_build(void *data, unsigned int capacity, const struct arp_packet *packet)
{
	unsigned char *bytes = (unsigned char *)data;

	if (capacity < ARP_PACKET_SIZE) {
		return 0;
	}

	be16_write(bytes, ARP_HARDWARE_ETHERNET);
	be16_write(bytes + 2, ARP_PROTOCOL_IPV4);
	bytes[4] = MAC_SIZE;
	bytes[5] = IPV4_SIZE;
	be16_write(bytes + 6, packet->operation);
	memcpy(bytes + 8, packet->sender_mac.bytes, MAC_SIZE);
	memcpy(bytes + 14, packet->sender_ip.bytes, IPV4_SIZE);
	memcpy(bytes + 18, packet->target_mac.bytes, MAC_SIZE);
	memcpy(bytes + 24, packet->target_ip.bytes, IPV4_SIZE);

	return ARP_PACKET_SIZE;
}

unsigned int arp_frame(void *frame, unsigned int capacity, const struct arp_packet *packet)
{
	struct ethernet_header header;
	unsigned char body[ARP_PACKET_SIZE];

	if (arp_build(body, sizeof(body), packet) == 0) {
		return 0;
	}

	if (packet->operation == ARP_REQUEST) {
		memset(header.destination.bytes, 0xFF, MAC_SIZE);
	} else {
		header.destination = packet->target_mac;
	}

	header.source = packet->sender_mac;
	header.type = ETHERTYPE_ARP;

	return ethernet_build(frame, capacity, &header, body, sizeof(body));
}

void arp_make_request(struct arp_packet *packet, const struct mac_addr *own_mac,
		const struct ipv4_addr *own_ip, const struct ipv4_addr *target_ip)
{
	packet->operation = ARP_REQUEST;
	packet->sender_mac = *own_mac;
	packet->sender_ip = *own_ip;
	memset(packet->target_mac.bytes, 0, MAC_SIZE);
	packet->target_ip = *target_ip;
}

void arp_make_reply(struct arp_packet *packet, const struct mac_addr *own_mac,
		const struct ipv4_addr *own_ip, const struct arp_packet *request)
{
	struct arp_packet result;

	result.operation = ARP_REPLY;
	result.sender_mac = *own_mac;
	result.sender_ip = *own_ip;
	result.target_mac = request->sender_mac;
	result.target_ip = request->sender_ip;

	*packet = result;
}

void arp_table_init(struct arp_table *table)
{
	memset(table, 0, sizeof(*table));
}

void arp_table_update(struct arp_table *table, const struct ipv4_addr *ip,
		const struct mac_addr *mac, unsigned int now)
{
	struct arp_entry *target = 0;
	unsigned int oldest_age = 0;
	int i;

	for (i = 0; i < ARP_TABLE_SIZE; i++) {
		struct arp_entry *entry = &table->entries[i];

		if (entry->used && ipv4_equal(&entry->ip, ip)) {
			target = entry;
			break;
		}
	}

	if (target == 0) {
		for (i = 0; i < ARP_TABLE_SIZE; i++) {
			if (!table->entries[i].used) {
				target = &table->entries[i];
				break;
			}
		}
	}

	if (target == 0) {
		for (i = 0; i < ARP_TABLE_SIZE; i++) {
			struct arp_entry *entry = &table->entries[i];
			unsigned int age = now - entry->updated;

			if (target == 0 || age > oldest_age) {
				target = entry;
				oldest_age = age;
			}
		}
	}

	target->ip = *ip;
	target->mac = *mac;
	target->updated = now;
	target->used = 1;
}

int arp_table_lookup(struct arp_table *table, const struct ipv4_addr *ip,
		struct mac_addr *mac, unsigned int now)
{
	int i;

	for (i = 0; i < ARP_TABLE_SIZE; i++) {
		struct arp_entry *entry = &table->entries[i];

		if (!entry->used || !ipv4_equal(&entry->ip, ip)) {
			continue;
		}

		if (now - entry->updated >= ARP_ENTRY_LIFETIME_MS) {
			entry->used = 0;
			return -1;
		}

		*mac = entry->mac;
		return 0;
	}

	return -1;
}

void arp_table_remove(struct arp_table *table, const struct ipv4_addr *ip)
{
	int i;

	for (i = 0; i < ARP_TABLE_SIZE; i++) {
		struct arp_entry *entry = &table->entries[i];

		if (entry->used && ipv4_equal(&entry->ip, ip)) {
			entry->used = 0;
		}
	}
}

void arp_table_expire(struct arp_table *table, unsigned int now)
{
	int i;

	for (i = 0; i < ARP_TABLE_SIZE; i++) {
		struct arp_entry *entry = &table->entries[i];

		if (entry->used && now - entry->updated >= ARP_ENTRY_LIFETIME_MS) {
			entry->used = 0;
		}
	}
}

static int sender_is_valid(const struct arp_packet *packet, const struct ipv4_addr *own_ip)
{
	unsigned int ip = ipv4_to_u32(&packet->sender_ip);

	if (ip == 0 || ip == 0xFFFFFFFFu) {
		return 0;
	}

	if (ipv4_equal(&packet->sender_ip, own_ip)) {
		return 0;
	}

	if (mac_is_zero(&packet->sender_mac) || mac_is_multicast(&packet->sender_mac)) {
		return 0;
	}

	return 1;
}

int arp_handle(struct arp_table *table, const struct arp_packet *packet,
		const struct mac_addr *own_mac, const struct ipv4_addr *own_ip,
		unsigned int now, struct arp_packet *reply)
{
	struct mac_addr known;
	int for_us = ipv4_equal(&packet->target_ip, own_ip);
	int valid = sender_is_valid(packet, own_ip);

	if (valid && (for_us || arp_table_lookup(table, &packet->sender_ip, &known, now) == 0)) {
		arp_table_update(table, &packet->sender_ip, &packet->sender_mac, now);
	}

	if (packet->operation == ARP_REQUEST && for_us
		&& !mac_is_zero(&packet->sender_mac) && !mac_is_multicast(&packet->sender_mac)
		&& !ipv4_equal(&packet->sender_ip, own_ip)) {
		arp_make_reply(reply, own_mac, own_ip, packet);
		return 1;
	}

	return 0;
}
