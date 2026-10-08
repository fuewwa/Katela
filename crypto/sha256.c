#include "sha256.h"
#include "mem.h"

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define BIG_SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define BIG_SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SMALL_SIGMA0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SMALL_SIGMA1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static const unsigned int constants[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void transform(struct sha256_context *context, const unsigned char *block)
{
	unsigned int w[64];
	unsigned int a, b, c, d, e, f, g, h;
	unsigned int t1, t2;
	int i;

	for (i = 0; i < 16; i++) {
		w[i] = ((unsigned int)block[i * 4] << 24)
			 | ((unsigned int)block[i * 4 + 1] << 16)
			 | ((unsigned int)block[i * 4 + 2] << 8)
			 | (unsigned int)block[i * 4 + 3];
	}

	for (i = 16; i < 64; i++) {
		w[i] = SMALL_SIGMA1(w[i - 2]) + w[i - 7] + SMALL_SIGMA0(w[i - 15]) + w[i - 16];
	}

	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];
	f = context->state[5];
	g = context->state[6];
	h = context->state[7];

	for (i = 0; i < 64; i++) {
		t1 = h + BIG_SIGMA1(e) + CH(e, f, g) + constants[i] + w[i];
		t2 = BIG_SIGMA0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
	context->state[5] += f;
	context->state[6] += g;
	context->state[7] += h;
}

static void store_big_endian(unsigned char *out, unsigned int value)
{
	out[0] = (unsigned char)(value >> 24);
	out[1] = (unsigned char)(value >> 16);
	out[2] = (unsigned char)(value >> 8);
	out[3] = (unsigned char)value;
}

void sha256_init(struct sha256_context *context)
{
	context->state[0] = 0x6a09e667;
	context->state[1] = 0xbb67ae85;
	context->state[2] = 0x3c6ef372;
	context->state[3] = 0xa54ff53a;
	context->state[4] = 0x510e527f;
	context->state[5] = 0x9b05688c;
	context->state[6] = 0x1f83d9ab;
	context->state[7] = 0x5be0cd19;
	context->bytes_low = 0;
	context->bytes_high = 0;
	context->buffer_length = 0;
}

void sha256_update(struct sha256_context *context, const void *data, unsigned int size)
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

		if (context->buffer_length == 0 && size >= SHA256_BLOCK_SIZE) {
			transform(context, bytes);
			bytes += SHA256_BLOCK_SIZE;
			size -= SHA256_BLOCK_SIZE;
			continue;
		}

		space = SHA256_BLOCK_SIZE - context->buffer_length;
		chunk = size < space ? size : space;

		memcpy(context->buffer + context->buffer_length, bytes, chunk);
		context->buffer_length += chunk;
		bytes += chunk;
		size -= chunk;

		if (context->buffer_length == SHA256_BLOCK_SIZE) {
			transform(context, context->buffer);
			context->buffer_length = 0;
		}
	}
}

void sha256_final(struct sha256_context *context, unsigned char *digest)
{
	unsigned int bits_high = (context->bytes_high << 3) | (context->bytes_low >> 29);
	unsigned int bits_low = context->bytes_low << 3;
	int i;

	context->buffer[context->buffer_length++] = 0x80;

	if (context->buffer_length > 56) {
		memset(context->buffer + context->buffer_length, 0, SHA256_BLOCK_SIZE - context->buffer_length);
		transform(context, context->buffer);
		context->buffer_length = 0;
	}

	memset(context->buffer + context->buffer_length, 0, 56 - context->buffer_length);
	store_big_endian(context->buffer + 56, bits_high);
	store_big_endian(context->buffer + 60, bits_low);
	transform(context, context->buffer);

	for (i = 0; i < 8; i++) {
		store_big_endian(digest + i * 4, context->state[i]);
	}

	memset(context, 0, sizeof(*context));
}

void sha256(const void *data, unsigned int size, unsigned char *digest)
{
	struct sha256_context context;

	sha256_init(&context);
	sha256_update(&context, data, size);
	sha256_final(&context, digest);
}

int sha256_equal(const unsigned char *a, const unsigned char *b)
{
	unsigned char difference = 0;
	int i;

	for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
		difference |= a[i] ^ b[i];
	}

	return difference == 0;
}
