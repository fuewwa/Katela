#include "udp.h"
#include "ipv4.h"
#include "checksum.h"
#include "byteorder.h"
#include "mem.h"

int udp_parse(const void *data, unsigned int size, const struct ipv4_addr *source,
		const struct ipv4_addr *destination, struct udp_header *header,
		const unsigned char **payload, unsigned int *payload_size)
{
	const unsigned char *bytes = (const unsigned char *)data;
	unsigned int length;

	if (size < UDP_HEADER_SIZE) {
		return UDP_ERR_MALFORMED;
	}

	length = be16_read(bytes + 4);
	if (length < UDP_HEADER_SIZE || length > size) {
		return UDP_ERR_MALFORMED;
	}

	if (be16_read(bytes + 6) != 0) {
		unsigned int sum = ipv4_pseudo_sum(source, destination, IPV4_PROTOCOL_UDP, length);

		sum = checksum_add(sum, bytes, length);
		if (checksum_finish(sum) != 0) {
			return UDP_ERR_CHECKSUM;
		}
	}

	header->source_port = be16_read(bytes);
	header->destination_port = be16_read(bytes + 2);
	header->length = (unsigned short)length;
	*payload = bytes + UDP_HEADER_SIZE;
	*payload_size = length - UDP_HEADER_SIZE;

	return 0;
}

unsigned int udp_build(void *data, unsigned int capacity, const struct ipv4_addr *source,
		const struct ipv4_addr *destination, unsigned short source_port,
		unsigned short destination_port, const void *payload, unsigned int payload_size)
{
	unsigned char *bytes = (unsigned char *)data;
	unsigned int total;
	unsigned int sum;
	unsigned short result;

	if (payload_size > IPV4_PACKET_MAX - IPV4_HEADER_SIZE - UDP_HEADER_SIZE) {
		return 0;
	}

	total = UDP_HEADER_SIZE + payload_size;
	if (capacity < total) {
		return 0;
	}

	be16_write(bytes, source_port);
	be16_write(bytes + 2, destination_port);
	be16_write(bytes + 4, (unsigned short)total);
	be16_write(bytes + 6, 0);
	memcpy(bytes + UDP_HEADER_SIZE, payload, payload_size);

	sum = ipv4_pseudo_sum(source, destination, IPV4_PROTOCOL_UDP, total);
	sum = checksum_add(sum, bytes, total);
	result = checksum_finish(sum);
	if (result == 0) {
		result = 0xFFFF;
	}
	be16_write(bytes + 6, result);

	return total;
}
