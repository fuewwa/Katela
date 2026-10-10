#include "sha1.h"
#include "mem.h"

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void transform(struct sha1_context *context, const unsigned char *block)
{
	unsigned int w[80];
	unsigned int a, b, c, d, e;
	unsigned int f, k, temp;
	int i;

	for (i = 0; i < 16; i++) {
		w[i] = ((unsigned int)block[i * 4] << 24)
			 | ((unsigned int)block[i * 4 + 1] << 16)
			 | ((unsigned int)block[i * 4 + 2] << 8)
			 | (unsigned int)block[i * 4 + 3];
	}

	for (i = 16; i < 80; i++) {
		w[i] = ROTL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
	}

	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];

	for (i = 0; i < 80; i++) {
		if (i < 20) {
			f = (b & c) | (~b & d);
			k = 0x5a827999;
		} else if (i < 40) {
			f = b ^ c ^ d;
			k = 0x6ed9eba1;
		} else if (i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8f1bbcdc;
		} else {
			f = b ^ c ^ d;
			k = 0xca62c1d6;
		}

		temp = ROTL(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = ROTL(b, 30);
		b = a;
		a = temp;
	}

	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
}

static void store_big_endian(unsigned char *out, unsigned int value)
{
	out[0] = (unsigned char)(value >> 24);
	out[1] = (unsigned char)(value >> 16);
	out[2] = (unsigned char)(value >> 8);
	out[3] = (unsigned char)value;
}

void sha1_init(struct sha1_context *context)
{
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
	context->state[4] = 0xc3d2e1f0;
	context->bytes_low = 0;
	context->bytes_high = 0;
	context->buffer_length = 0;
}

void sha1_update(struct sha1_context *context, const void *data, unsigned int size)
{
	const unsigned char *bytes = (const unsigned char *)data;
	unsigned int previous = context->bytes_low;

	context->bytes_low += size;
	if (context->bytes_low < previous) {
		context->bytes_high++;
	}

	while (size > 0) {
		unsigned int space;
		unsigned int chunk;

		if (context->buffer_length == 0 && size >= SHA1_BLOCK_SIZE) {
			transform(context, bytes);
			bytes += SHA1_BLOCK_SIZE;
			size -= SHA1_BLOCK_SIZE;
			continue;
		}

		space = SHA1_BLOCK_SIZE - context->buffer_length;
		chunk = size < space ? size : space;

		memcpy(context->buffer + context->buffer_length, bytes, chunk);
		context->buffer_length += chunk;
		bytes += chunk;
		size -= chunk;

		if (context->buffer_length == SHA1_BLOCK_SIZE) {
			transform(context, context->buffer);
			context->buffer_length = 0;
		}
	}
}

void sha1_final(struct sha1_context *context, unsigned char *digest)
{
	unsigned int bits_high = (context->bytes_high << 3) | (context->bytes_low >> 29);
	unsigned int bits_low = context->bytes_low << 3;
	int i;

	context->buffer[context->buffer_length++] = 0x80;

	if (context->buffer_length > 56) {
		memset(context->buffer + context->buffer_length, 0, SHA1_BLOCK_SIZE - context->buffer_length);
		transform(context, context->buffer);
		context->buffer_length = 0;
	}

	memset(context->buffer + context->buffer_length, 0, 56 - context->buffer_length);
	store_big_endian(context->buffer + 56, bits_high);
	store_big_endian(context->buffer + 60, bits_low);
	transform(context, context->buffer);

	for (i = 0; i < 5; i++) {
		store_big_endian(digest + i * 4, context->state[i]);
	}

	memset(context, 0, sizeof(*context));
}

void sha1(const void *data, unsigned int size, unsigned char *digest)
{
	struct sha1_context context;

	sha1_init(&context);
	sha1_update(&context, data, size);
	sha1_final(&context, digest);
}

int sha1_equal(const unsigned char *a, const unsigned char *b)
{
	unsigned char difference = 0;
	int i;

	for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
		difference |= a[i] ^ b[i];
	}

	return difference == 0;
}
