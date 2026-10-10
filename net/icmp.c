#include "icmp.h"
#include "ipv4.h"
#include "checksum.h"
#include "byteorder.h"
#include "mem.h"

int icmp_parse(const void *data, unsigned int size, struct icmp_message *message)
{
	const unsigned char *bytes = (const unsigned char *)data;

	if (size < ICMP_HEADER_SIZE) {
		return -1;
	}

	if (checksum(bytes, size) != 0) {
		return -1;
	}

	message->type = bytes[0];
	message->code = bytes[1];
	message->id = be16_read(bytes + 4);
	message->sequence = be16_read(bytes + 6);
	message->payload = bytes + ICMP_HEADER_SIZE;
	message->payload_size = size - ICMP_HEADER_SIZE;

	return 0;
}

unsigned int icmp_build(void *data, unsigned int capacity, unsigned char type, unsigned char code,
		unsigned short id, unsigned short sequence, const void *payload, unsigned int payload_size)
{
	unsigned char *bytes = (unsigned char *)data;
	unsigned int total;

	if (payload_size > IPV4_PACKET_MAX - IPV4_HEADER_SIZE - ICMP_HEADER_SIZE) {
		return 0;
	}

	total = ICMP_HEADER_SIZE + payload_size;
	if (capacity < total) {
		return 0;
	}

	bytes[0] = type;
	bytes[1] = code;
	be16_write(bytes + 2, 0);
	be16_write(bytes + 4, id);
	be16_write(bytes + 6, sequence);
	memcpy(bytes + ICMP_HEADER_SIZE, payload, payload_size);
	be16_write(bytes + 2, checksum(bytes, total));

	return total;
}

unsigned int icmp_echo_reply(void *data, unsigned int capacity, const struct icmp_message *request)
{
	if (request->type != ICMP_ECHO_REQUEST) {
		return 0;
	}

	return icmp_build(data, capacity, ICMP_ECHO_REPLY, 0, request->id, request->sequence,
			request->payload, request->payload_size);
}

unsigned int icmp_build_error(void *data, unsigned int capacity, unsigned char type, unsigned char code,
		const void *original, unsigned int original_size)
{
	const unsigned char *bytes = (const unsigned char *)original;
	unsigned int header_length;
	unsigned int quoted;

	if (original_size < IPV4_HEADER_SIZE) {
		return 0;
	}

	header_length = (unsigned int)(bytes[0] & 0x0F) * 4;
	if (header_length < IPV4_HEADER_SIZE) {
		return 0;
	}

	quoted = header_length + 8;
	if (quoted > original_size) {
		quoted = original_size;
	}

	return icmp_build(data, capacity, type, code, 0, 0, original, quoted);
}
