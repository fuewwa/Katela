#ifndef SHA256_H
#define SHA256_H

#define SHA256_DIGEST_SIZE 32
#define SHA256_BLOCK_SIZE 64

struct sha256_context {
	unsigned int state[8];
	unsigned int bytes_low;
	unsigned int bytes_high;
	unsigned int buffer_length;
	unsigned char buffer[SHA256_BLOCK_SIZE];
};

void sha256_init(struct sha256_context *context);
void sha256_update(struct sha256_context *context, const void *data, unsigned int size);
void sha256_final(struct sha256_context *context, unsigned char *digest);
void sha256(const void *data, unsigned int size, unsigned char *digest);
int sha256_equal(const unsigned char *a, const unsigned char *b);

#endif
