#include "dns.h"
#include "byteorder.h"

#define HEADER_SIZE 12
#define WIRE_NAME_MAX 255
#define LABEL_MAX 63

#define FLAG_RESPONSE 0x8000
#define FLAG_OPCODE_MASK 0x7800
#define FLAG_RECURSION_DESIRED 0x0100
#define FLAG_RCODE_MASK 0x000F

#define RCODE_NXDOMAIN 3

#define TYPE_A 1
#define CLASS_IN 1

static int name_char_valid(unsigned char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
		|| c == '-' || c == '_';
}

static unsigned char lower(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') ? (unsigned char)(c + ('a' - 'A')) : c;
}

static unsigned int name_encode(unsigned char *out, unsigned int limit, const char *name)
{
	unsigned int length = 0;
	unsigned int i = 0;

	if (limit > WIRE_NAME_MAX) {
		limit = WIRE_NAME_MAX;
	}

	for (;;) {
		unsigned int start = i;
		unsigned int label_length;
		unsigned int j;

		while (name[i] != 0 && name[i] != '.') {
			if (!name_char_valid((unsigned char)name[i])) {
				return 0;
			}
			i++;
		}

		label_length = i - start;
		if (label_length == 0 || label_length > LABEL_MAX) {
			return 0;
		}

		if (length + 1 + label_length + 1 > limit) {
			return 0;
		}

		out[length] = (unsigned char)label_length;
		for (j = 0; j < label_length; j++) {
			out[length + 1 + j] = (unsigned char)name[start + j];
		}
		length += 1 + label_length;

		if (name[i] == 0) {
			break;
		}

		i++;
		if (name[i] == 0) {
			break;
		}
	}

	out[length++] = 0;
	return length;
}

static int name_skip(const unsigned char *bytes, unsigned int size, unsigned int *offset)
{
	for (;;) {
		unsigned int length;

		if (*offset >= size) {
			return -1;
		}

		length = bytes[*offset];

		if (length == 0) {
			*offset += 1;
			return 0;
		}

		if ((length & 0xC0) == 0xC0) {
			if (*offset + 2 > size) {
				return -1;
			}
			*offset += 2;
			return 0;
		}

		if ((length & 0xC0) != 0) {
			return -1;
		}

		*offset += 1 + length;
		if (*offset > size) {
			return -1;
		}
	}
}

unsigned int dns_query_build(void *data, unsigned int capacity, unsigned short id, const char *name)
{
	unsigned char *bytes = (unsigned char *)data;
	unsigned int name_length;
	unsigned int i;

	if (capacity < HEADER_SIZE + 2 + 4) {
		return 0;
	}

	name_length = name_encode(bytes + HEADER_SIZE, capacity - HEADER_SIZE - 4, name);
	if (name_length == 0) {
		return 0;
	}

	for (i = 0; i < HEADER_SIZE; i++) {
		bytes[i] = 0;
	}

	be16_write(bytes, id);
	be16_write(bytes + 2, FLAG_RECURSION_DESIRED);
	be16_write(bytes + 4, 1);
	be16_write(bytes + HEADER_SIZE + name_length, TYPE_A);
	be16_write(bytes + HEADER_SIZE + name_length + 2, CLASS_IN);

	return HEADER_SIZE + name_length + 4;
}

int dns_response_parse(const void *data, unsigned int size, unsigned short id, const char *name,
		struct ipv4_addr *address)
{
	const unsigned char *bytes = (const unsigned char *)data;
	unsigned char expected[WIRE_NAME_MAX + 1];
	unsigned int expected_length;
	unsigned int flags;
	unsigned int answers;
	unsigned int offset;
	unsigned int i;

	if (size < HEADER_SIZE) {
		return DNS_ERR_MALFORMED;
	}

	if (be16_read(bytes) != id) {
		return DNS_ERR_MISMATCH;
	}

	flags = be16_read(bytes + 2);
	if (!(flags & FLAG_RESPONSE) || (flags & FLAG_OPCODE_MASK) != 0) {
		return DNS_ERR_MISMATCH;
	}

	if (be16_read(bytes + 4) != 1) {
		return DNS_ERR_MALFORMED;
	}

	expected_length = name_encode(expected, sizeof(expected), name);
	if (expected_length == 0) {
		return DNS_ERR_NAME;
	}

	offset = HEADER_SIZE;
	if (offset + expected_length + 4 > size) {
		return DNS_ERR_MALFORMED;
	}

	for (i = 0; i < expected_length; i++) {
		if (lower(bytes[offset + i]) != lower(expected[i])) {
			return DNS_ERR_MISMATCH;
		}
	}
	offset += expected_length;

	if (be16_read(bytes + offset) != TYPE_A || be16_read(bytes + offset + 2) != CLASS_IN) {
		return DNS_ERR_MISMATCH;
	}
	offset += 4;

	if ((flags & FLAG_RCODE_MASK) == RCODE_NXDOMAIN) {
		return DNS_ERR_NXDOMAIN;
	}

	if ((flags & FLAG_RCODE_MASK) != 0) {
		return DNS_ERR_SERVER;
	}

	answers = be16_read(bytes + 6);

	for (i = 0; i < answers; i++) {
		unsigned int type;
		unsigned int class_code;
		unsigned int length;

		if (name_skip(bytes, size, &offset) != 0) {
			return DNS_ERR_MALFORMED;
		}

		if (offset + 10 > size) {
			return DNS_ERR_MALFORMED;
		}

		type = be16_read(bytes + offset);
		class_code = be16_read(bytes + offset + 2);
		length = be16_read(bytes + offset + 8);
		offset += 10;

		if (offset + length > size) {
			return DNS_ERR_MALFORMED;
		}

		if (type == TYPE_A && class_code == CLASS_IN && length == IPV4_SIZE) {
			unsigned int j;

			for (j = 0; j < IPV4_SIZE; j++) {
				address->bytes[j] = bytes[offset + j];
			}
			return 0;
		}

		offset += length;
	}

	return DNS_ERR_NO_ANSWER;
}
