#include "addr.h"

static int hex_value(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}

	return -1;
}

int mac_parse(const char *text, struct mac_addr *addr)
{
	struct mac_addr result;
	int i;

	for (i = 0; i < MAC_SIZE; i++) {
		int high = hex_value(text[0]);
		int low;

		if (high < 0) {
			return -1;
		}

		low = hex_value(text[1]);
		if (low < 0) {
			return -1;
		}

		result.bytes[i] = (unsigned char)((high << 4) | low);
		text += 2;

		if (i < MAC_SIZE - 1) {
			if (*text != ':' && *text != '-') {
				return -1;
			}
			text++;
		}
	}

	if (*text != '\0') {
		return -1;
	}

	*addr = result;
	return 0;
}

void mac_format(const struct mac_addr *addr, char *out)
{
	static const char digits[] = "0123456789abcdef";
	int i;

	for (i = 0; i < MAC_SIZE; i++) {
		*out++ = digits[addr->bytes[i] >> 4];
		*out++ = digits[addr->bytes[i] & 0x0F];

		if (i < MAC_SIZE - 1) {
			*out++ = ':';
		}
	}

	*out = '\0';
}

int mac_equal(const struct mac_addr *a, const struct mac_addr *b)
{
	unsigned char difference = 0;
	int i;

	for (i = 0; i < MAC_SIZE; i++) {
		difference |= a->bytes[i] ^ b->bytes[i];
	}

	return difference == 0;
}

int mac_is_zero(const struct mac_addr *addr)
{
	unsigned char bits = 0;
	int i;

	for (i = 0; i < MAC_SIZE; i++) {
		bits |= addr->bytes[i];
	}

	return bits == 0;
}

int mac_is_broadcast(const struct mac_addr *addr)
{
	unsigned char bits = 0xFF;
	int i;

	for (i = 0; i < MAC_SIZE; i++) {
		bits &= addr->bytes[i];
	}

	return bits == 0xFF;
}

int mac_is_multicast(const struct mac_addr *addr)
{
	return (addr->bytes[0] & 0x01) != 0;
}

int ipv4_parse(const char *text, struct ipv4_addr *addr)
{
	struct ipv4_addr result;
	int i;

	for (i = 0; i < IPV4_SIZE; i++) {
		unsigned int value = 0;
		int digits = 0;

		while (*text >= '0' && *text <= '9') {
			value = value * 10 + (unsigned int)(*text - '0');
			digits++;
			text++;

			if (digits > 3 || value > 255) {
				return -1;
			}
		}

		if (digits == 0) {
			return -1;
		}

		result.bytes[i] = (unsigned char)value;

		if (i < IPV4_SIZE - 1) {
			if (*text != '.') {
				return -1;
			}
			text++;
		}
	}

	if (*text != '\0') {
		return -1;
	}

	*addr = result;
	return 0;
}

void ipv4_format(const struct ipv4_addr *addr, char *out)
{
	int i;

	for (i = 0; i < IPV4_SIZE; i++) {
		unsigned char value = addr->bytes[i];

		if (value >= 100) {
			*out++ = (char)('0' + value / 100);
		}
		if (value >= 10) {
			*out++ = (char)('0' + (value / 10) % 10);
		}
		*out++ = (char)('0' + value % 10);

		if (i < IPV4_SIZE - 1) {
			*out++ = '.';
		}
	}

	*out = '\0';
}

int ipv4_equal(const struct ipv4_addr *a, const struct ipv4_addr *b)
{
	unsigned char difference = 0;
	int i;

	for (i = 0; i < IPV4_SIZE; i++) {
		difference |= a->bytes[i] ^ b->bytes[i];
	}

	return difference == 0;
}

unsigned int ipv4_to_u32(const struct ipv4_addr *addr)
{
	return ((unsigned int)addr->bytes[0] << 24)
		 | ((unsigned int)addr->bytes[1] << 16)
		 | ((unsigned int)addr->bytes[2] << 8)
		 | (unsigned int)addr->bytes[3];
}

void ipv4_from_u32(struct ipv4_addr *addr, unsigned int value)
{
	addr->bytes[0] = (unsigned char)(value >> 24);
	addr->bytes[1] = (unsigned char)(value >> 16);
	addr->bytes[2] = (unsigned char)(value >> 8);
	addr->bytes[3] = (unsigned char)value;
}
