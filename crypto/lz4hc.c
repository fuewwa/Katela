#include "lz4hc.h"
#include "lz4.h"
#include "mem.h"

#define HC_HASH_BITS 15
#define HC_WINDOW_SIZE 65536

static unsigned int head_table[1 << HC_HASH_BITS];
static unsigned short chain_table[HC_WINDOW_SIZE];

static unsigned int read_word(const unsigned char *p)
{
	return (unsigned int)p[0]
		 | ((unsigned int)p[1] << 8)
		 | ((unsigned int)p[2] << 16)
		 | ((unsigned int)p[3] << 24);
}

static unsigned int hash_word(unsigned int word)
{
	return (word * 2654435761u) >> (32 - HC_HASH_BITS);
}

static void insert_position(const unsigned char *src, unsigned int pos)
{
	unsigned int slot = hash_word(read_word(src + pos));
	unsigned int previous = head_table[slot];
	unsigned int delta = 0;

	if (previous != 0 && pos - (previous - 1) <= LZ4_MAX_OFFSET) {
		delta = pos - (previous - 1);
	}

	chain_table[pos & (HC_WINDOW_SIZE - 1)] = (unsigned short)delta;
	head_table[slot] = pos + 1;
}

static void update_chains(const unsigned char *src, unsigned int *next_update, unsigned int target)
{
	while (*next_update < target) {
		insert_position(src, *next_update);
		(*next_update)++;
	}
}

static unsigned int find_match(const unsigned char *src, unsigned int pos, unsigned int size,
							   unsigned int attempts, unsigned int *offset)
{
	unsigned int candidate = head_table[hash_word(read_word(src + pos))];
	unsigned int limit = size - LZ4_LAST_LITERALS - pos;
	unsigned int best = 0;

	while (candidate != 0 && attempts > 0) {
		unsigned int match = candidate - 1;
		unsigned int distance = pos - match;
		unsigned int step;

		if (distance > LZ4_MAX_OFFSET) {
			break;
		}

		if (best == 0 || src[match + best] == src[pos + best]) {
			unsigned int length = 0;

			while (length < limit && src[match + length] == src[pos + length]) {
				length++;
			}

			if (length >= LZ4_MIN_MATCH && length > best) {
				best = length;
				*offset = distance;

				if (best == limit) {
					break;
				}
			}
		}

		step = chain_table[match & (HC_WINDOW_SIZE - 1)];

		if (step == 0) {
			break;
		}

		candidate = match - step + 1;
		attempts--;
	}

	return best;
}

int lz4hc_compress(const void *source, unsigned int size, void *dest, unsigned int capacity, int level)
{
	const unsigned char *src = (const unsigned char *)source;
	unsigned char *dst = (unsigned char *)dest;
	unsigned char *out = dst;
	unsigned char *out_end = dst + capacity;
	unsigned int anchor = 0;
	unsigned int pos = 0;
	unsigned int next_update = 0;
	unsigned int attempts;

	if (size > LZ4_MAX_INPUT_SIZE || capacity > 0x7FFFFFFFu) {
		return -1;
	}

	if (level < LZ4HC_MIN_LEVEL) {
		level = LZ4HC_MIN_LEVEL;
	}

	if (level > LZ4HC_MAX_LEVEL) {
		level = LZ4HC_MAX_LEVEL;
	}

	attempts = 1u << level;
	memset(head_table, 0, sizeof(head_table));

	while (pos + LZ4_MATCH_FIND_LIMIT <= size) {
		unsigned int offset = 0;
		unsigned int length;

		update_chains(src, &next_update, pos);
		length = find_match(src, pos, size, attempts, &offset);

		if (length == 0) {
			pos++;
			continue;
		}

		while (pos + 1 + LZ4_MATCH_FIND_LIMIT <= size) {
			unsigned int next_offset = 0;
			unsigned int next_length;

			update_chains(src, &next_update, pos + 1);
			next_length = find_match(src, pos + 1, size, attempts, &next_offset);

			if (next_length <= length + 1) {
				break;
			}

			pos++;
			length = next_length;
			offset = next_offset;
		}

		if (!lz4_write_sequence(&out, out_end, src + anchor, pos - anchor, offset, length)) {
			return -1;
		}

		pos += length;
		anchor = pos;
	}

	if (!lz4_write_sequence(&out, out_end, src + anchor, size - anchor, 0, 0)) {
		return -1;
	}

	return (int)(out - dst);
}
