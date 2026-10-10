#ifndef SHA1_H
#define SHA1_H

#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE 64

struct sha1_context {
	unsigned int state[5];
	unsigned int bytes_low;
	unsigned int bytes_high;
	unsigned int buffer_length;
	unsigned char buffer[SHA1_BLOCK_SIZE];
};

void sha1_init(struct sha1_context *context);
void sha1_update(struct sha1_context *context, const void *data, unsigned int size);
void sha1_final(struct sha1_context *context, unsigned char *digest);
void sha1(const void *data, unsigned int size, unsigned char *digest);
int sha1_equal(const unsigned char *a, const unsigned char *b);

#endif
