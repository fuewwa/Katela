#include "ipv4.h"
#include "checksum.h"
#include "byteorder.h"
#include "mem.h"

void ipv4_header_init(struct ipv4_header *header, const struct ipv4_addr *source,
		const struct ipv4_addr *destination, unsigned char protocol)
{
	header->header_length = IPV4_HEADER_SIZE;
	header->tos = 0;
	header->total_length = 0;
	header->id = 0;
	header->flags_fragment = IPV4_FLAG_DONT_FRAGMENT;
	header->ttl = IPV4_DEFAULT_TTL;
	header->protocol = protocol;
	header->source = *source;
	header->destination = *destination;
}

int ipv4_packet_parse(const void *packet, unsigned int size, struct ipv4_header *header,
		const unsigned char **payload, unsigned int *payload_size)
{
	const unsigned char *bytes = (const unsigned char *)packet;
	unsigned int header_length;
	unsigned int total_length;

	if (size < IPV4_HEADER_SIZE) {
		return IPV4_ERR_MALFORMED;
	}

	if ((bytes[0] >> 4) != 4) {
		return IPV4_ERR_MALFORMED;
	}

	header_length = (unsigned int)(bytes[0] & 0x0F) * 4;
	if (header_length < IPV4_HEADER_SIZE || header_length > size) {
		return IPV4_ERR_MALFORMED;
	}

	total_length = be16_read(bytes + 2);
	if (total_length < header_length || total_length > size) {
		return IPV4_ERR_MALFORMED;
	}

	if (checksum(bytes, header_length) != 0) {
		return IPV4_ERR_MALFORMED;
	}

	header->header_length = (unsigned char)header_length;
	header->tos = bytes[1];
	header->total_length = (unsigned short)total_length;
	header->id = be16_read(bytes + 4);
	header->flags_fragment = be16_read(bytes + 6);
	header->ttl = bytes[8];
	header->protocol = bytes[9];
	memcpy(header->source.bytes, bytes + 12, IPV4_SIZE);
	memcpy(header->destination.bytes, bytes + 16, IPV4_SIZE);

	if (header->flags_fragment & (IPV4_FLAG_MORE_FRAGMENTS | IPV4_FRAGMENT_OFFSET_MASK)) {
		return IPV4_ERR_FRAGMENT;
	}

	*payload = bytes + header_length;
	*payload_size = total_length - header_length;

	return 0;
}

unsigned int ipv4_packet_build(void *packet, unsigned int capacity, const struct ipv4_header *header,
		const void *payload, unsigned int payload_size)
{
	unsigned char *bytes = (unsigned char *)packet;
	unsigned int total;

	if (payload_size > IPV4_PACKET_MAX - IPV4_HEADER_SIZE) {
		return 0;
	}

	total = IPV4_HEADER_SIZE + payload_size;
	if (capacity < total) {
		return 0;
	}

	bytes[0] = 0x45;
	bytes[1] = header->tos;
	be16_write(bytes + 2, (unsigned short)total);
	be16_write(bytes + 4, header->id);
	be16_write(bytes + 6, header->flags_fragment);
	bytes[8] = header->ttl;
	bytes[9] = header->protocol;
	be16_write(bytes + 10, 0);
	memcpy(bytes + 12, header->source.bytes, IPV4_SIZE);
	memcpy(bytes + 16, header->destination.bytes, IPV4_SIZE);
	be16_write(bytes + 10, checksum(bytes, IPV4_HEADER_SIZE));
	memcpy(bytes + IPV4_HEADER_SIZE, payload, payload_size);

	return total;
}

unsigned int ipv4_pseudo_sum(const struct ipv4_addr *source, const struct ipv4_addr *destination,
		unsigned char protocol, unsigned int length)
{
	unsigned char pseudo[12];

	memcpy(pseudo, source->bytes, IPV4_SIZE);
	memcpy(pseudo + 4, destination->bytes, IPV4_SIZE);
	pseudo[8] = 0;
	pseudo[9] = protocol;
	be16_write(pseudo + 10, (unsigned short)length);

	return checksum_add(0, pseudo, sizeof(pseudo));
}

int ipv4_in_subnet(const struct ipv4_addr *a, const struct ipv4_addr *b, const struct ipv4_addr *mask)
{
	return ((ipv4_to_u32(a) ^ ipv4_to_u32(b)) & ipv4_to_u32(mask)) == 0;
}
