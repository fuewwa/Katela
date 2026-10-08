#include "lz4.h"
#include "mem.h"

#define LZ4_HASH_BITS 12

static unsigned int hash_table[1 << LZ4_HASH_BITS];

static unsigned int read_word(const unsigned char *p)
{
	return (unsigned int)p[0]
		 | ((unsigned int)p[1] << 8)
		 | ((unsigned int)p[2] << 16)
		 | ((unsigned int)p[3] << 24);
}

static unsigned int hash_word(unsigned int word)
{
	return (word * 2654435761u) >> (32 - LZ4_HASH_BITS);
}

static unsigned int extra_length_bytes(unsigned int value)
{
	if (value < 15) {
		return 0;
	}

	return (value - 15) / 255 + 1;
}

static void write_extra_length(unsigned char **out, unsigned int value)
{
	unsigned int remaining = value - 15;

	while (remaining >= 255) {
		*(*out)++ = 255;
		remaining -= 255;
	}

	*(*out)++ = (unsigned char)remaining;
}

int lz4_write_sequence(unsigned char **out, unsigned char *end,
					   const unsigned char *literals, unsigned int literal_length,
					   unsigned int offset, unsigned int match_length)
{
	unsigned int match_code = 0;
	unsigned int needed = 1 + literal_length + extra_length_bytes(literal_length);
	unsigned char *p;
	unsigned char token;

	if (match_length != 0) {
		match_code = match_length - LZ4_MIN_MATCH;
		needed += 2 + extra_length_bytes(match_code);
	}

	if ((unsigned int)(end - *out) < needed) {
		return 0;
	}

	p = *out;
	token = (unsigned char)((literal_length >= 15 ? 15 : literal_length) << 4);

	if (match_length != 0) {
		token |= (unsigned char)(match_code >= 15 ? 15 : match_code);
	}

	*p++ = token;

	if (literal_length >= 15) {
		write_extra_length(&p, literal_length);
	}

	memcpy(p, literals, literal_length);
	p += literal_length;

	if (match_length != 0) {
		*p++ = (unsigned char)(offset & 0xFF);
		*p++ = (unsigned char)(offset >> 8);

		if (match_code >= 15) {
			write_extra_length(&p, match_code);
		}
	}

	*out = p;
	return 1;
}

static int read_extra_length(const unsigned char **in, const unsigned char *end, unsigned int *value)
{
	unsigned int byte;

	do {
		if (*in >= end) {
			return 0;
		}

		byte = *(*in)++;

		if (*value > 0x7FFFFFFFu - 255u) {
			return 0;
		}

		*value += byte;
	} while (byte == 255);

	return 1;
}

unsigned int lz4_bound(unsigned int size)
{
	return size + size / 255 + 16;
}

int lz4_compress(const void *source, unsigned int size, void *dest, unsigned int capacity)
{
	const unsigned char *src = (const unsigned char *)source;
	unsigned char *dst = (unsigned char *)dest;
	unsigned char *out = dst;
	unsigned char *out_end = dst + capacity;
	unsigned int anchor = 0;
	unsigned int pos = 0;

	if (size > LZ4_MAX_INPUT_SIZE || capacity > 0x7FFFFFFFu) {
		return -1;
	}

	memset(hash_table, 0, sizeof(hash_table));

	while (pos + LZ4_MATCH_FIND_LIMIT <= size) {
		unsigned int word = read_word(src + pos);
		unsigned int slot = hash_word(word);
		unsigned int candidate = hash_table[slot];

		hash_table[slot] = pos + 1;

		if (candidate != 0) {
			candidate--;

			if (pos - candidate <= LZ4_MAX_OFFSET && read_word(src + candidate) == word) {
				unsigned int length = LZ4_MIN_MATCH;

				while (pos + length < size - LZ4_LAST_LITERALS
					   && src[candidate + length] == src[pos + length]) {
					length++;
				}

				if (!lz4_write_sequence(&out, out_end, src + anchor, pos - anchor, pos - candidate, length)) {
					return -1;
				}

				pos += length;
				anchor = pos;
				continue;
			}
		}

		pos++;
	}

	if (!lz4_write_sequence(&out, out_end, src + anchor, size - anchor, 0, 0)) {
		return -1;
	}

	return (int)(out - dst);
}

int lz4_decompress(const void *source, unsigned int size, void *dest, unsigned int capacity)
{
	const unsigned char *in = (const unsigned char *)source;
	const unsigned char *in_end = in + size;
	unsigned char *base = (unsigned char *)dest;
	unsigned char *out = base;
	unsigned char *out_end = base + capacity;

	if (size == 0 || size > 0x7FFFFFFFu || capacity > 0x7FFFFFFFu) {
		return -1;
	}

	while (1) {
		unsigned int token;
		unsigned int literal_length;
		unsigned int match_length;
		unsigned int offset;
		const unsigned char *match;

		if (in >= in_end) {
			return -1;
		}

		token = *in++;
		literal_length = token >> 4;

		if (literal_length == 15 && !read_extra_length(&in, in_end, &literal_length)) {
			return -1;
		}

		if ((unsigned int)(in_end - in) < literal_length || (unsigned int)(out_end - out) < literal_length) {
			return -1;
		}

		memcpy(out, in, literal_length);
		in += literal_length;
		out += literal_length;

		if (in == in_end) {
			break;
		}

		if (in_end - in < 2) {
			return -1;
		}

		offset = (unsigned int)in[0] | ((unsigned int)in[1] << 8);
		in += 2;

		if (offset == 0 || offset > (unsigned int)(out - base)) {
			return -1;
		}

		match_length = token & 15;

		if (match_length == 15 && !read_extra_length(&in, in_end, &match_length)) {
			return -1;
		}

		match_length += LZ4_MIN_MATCH;

		if ((unsigned int)(out_end - out) < match_length) {
			return -1;
		}

		match = out - offset;

		while (match_length > 0) {
			*out++ = *match++;
			match_length--;
		}
	}

	return (int)(out - base);
}
