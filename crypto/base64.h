#ifndef BASE64_H
#define BASE64_H

#define BASE64_ERROR (-1)

unsigned int base64_encoded_size(unsigned int size);
unsigned int base64_decoded_size(unsigned int length);
unsigned int base64_encode(const void *data, unsigned int size, char *output);
int base64_decode(const char *input, unsigned int length, void *output, unsigned int *output_size);

#endif
