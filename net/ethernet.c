#include "ethernet.h"
#include "byteorder.h"
#include "mem.h"

int ethernet_parse(const void *frame, unsigned int size, struct ethernet_header *header,
		const unsigned char **payload, unsigned int *payload_size)
{
	const unsigned char *bytes = (const unsigned char *)frame;

	if (size < ETHERNET_HEADER_SIZE || size > ETHERNET_FRAME_MAX) {
		return -1;
	}

	memcpy(header->destination.bytes, bytes, MAC_SIZE);
	memcpy(header->source.bytes, bytes + MAC_SIZE, MAC_SIZE);
	header->type = be16_read(bytes + 2 * MAC_SIZE);

	*payload = bytes + ETHERNET_HEADER_SIZE;
	*payload_size = size - ETHERNET_HEADER_SIZE;

	return 0;
}

unsigned int ethernet_build(void *frame, unsigned int capacity, const struct ethernet_header *header,
		const void *payload, unsigned int payload_size)
{
	unsigned char *bytes = (unsigned char *)frame;
	unsigned int size = ETHERNET_HEADER_SIZE + payload_size;

	if (payload_size > ETHERNET_MTU) {
		return 0;
	}

	if (size < ETHERNET_FRAME_MIN) {
		size = ETHERNET_FRAME_MIN;
	}

	if (capacity < size) {
		return 0;
	}

	memcpy(bytes, header->destination.bytes, MAC_SIZE);
	memcpy(bytes + MAC_SIZE, header->source.bytes, MAC_SIZE);
	be16_write(bytes + 2 * MAC_SIZE, header->type);
	memcpy(bytes + ETHERNET_HEADER_SIZE, payload, payload_size);
	memset(bytes + ETHERNET_HEADER_SIZE + payload_size, 0, size - ETHERNET_HEADER_SIZE - payload_size);

	return size;
}

int ethernet_for_us(const struct ethernet_header *header, const struct mac_addr *own)
{
	return mac_equal(&header->destination, own) || mac_is_broadcast(&header->destination);
}
