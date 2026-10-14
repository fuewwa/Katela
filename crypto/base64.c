#include "base64.h"

static const char alphabet[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static int symbol_value(char symbol)
{
	if (symbol >= 'A' && symbol <= 'Z') {
		return symbol - 'A';
	}
	if (symbol >= 'a' && symbol <= 'z') {
		return symbol - 'a' + 26;
	}
	if (symbol >= '0' && symbol <= '9') {
		return symbol - '0' + 52;
	}
	if (symbol == '+') {
		return 62;
	}
	if (symbol == '/') {
		return 63;
	}
	return BASE64_ERROR;
}

unsigned int base64_encoded_size(unsigned int size)
{
	return ((size + 2) / 3) * 4;
}

unsigned int base64_decoded_size(unsigned int length)
{
	return (length / 4) * 3;
}

unsigned int base64_encode(const void *data, unsigned int size, char *output)
{
	const unsigned char *input = (const unsigned char *)data;
	unsigned int written = 0;
	unsigned int i = 0;
	unsigned int group;

	while (size - i >= 3) {
		group = ((unsigned int)input[i] << 16)
			  | ((unsigned int)input[i + 1] << 8)
			  | (unsigned int)input[i + 2];
		output[written++] = alphabet[(group >> 18) & 0x3f];
		output[written++] = alphabet[(group >> 12) & 0x3f];
		output[written++] = alphabet[(group >> 6) & 0x3f];
		output[written++] = alphabet[group & 0x3f];
		i += 3;
	}

	if (size - i == 1) {
		group = (unsigned int)input[i] << 16;
		output[written++] = alphabet[(group >> 18) & 0x3f];
		output[written++] = alphabet[(group >> 12) & 0x3f];
		output[written++] = '=';
		output[written++] = '=';
	} else if (size - i == 2) {
		group = ((unsigned int)input[i] << 16)
			  | ((unsigned int)input[i + 1] << 8);
		output[written++] = alphabet[(group >> 18) & 0x3f];
		output[written++] = alphabet[(group >> 12) & 0x3f];
		output[written++] = alphabet[(group >> 6) & 0x3f];
		output[written++] = '=';
	}

	output[written] = '\0';
	return written;
}

int base64_decode(const char *input, unsigned int length, void *output, unsigned int *output_size)
{
	unsigned char *out = (unsigned char *)output;
	unsigned int written = 0;
	unsigned int i;
	unsigned int group;
	int values[4];
	int padding;
	int j;

	if (length % 4 != 0) {
		return BASE64_ERROR;
	}

	for (i = 0; i < length; i += 4) {
		padding = 0;

		for (j = 0; j < 4; j++) {
			if (input[i + j] == '=') {
				if (i + 4 != length || j < 2) {
					return BASE64_ERROR;
				}
				padding++;
				values[j] = 0;
				continue;
			}
			if (padding != 0) {
				return BASE64_ERROR;
			}
			values[j] = symbol_value(input[i + j]);
			if (values[j] == BASE64_ERROR) {
				return BASE64_ERROR;
			}
		}

		group = ((unsigned int)values[0] << 18)
			  | ((unsigned int)values[1] << 12)
			  | ((unsigned int)values[2] << 6)
			  | (unsigned int)values[3];

		out[written++] = (unsigned char)(group >> 16);
		if (padding < 2) {
			out[written++] = (unsigned char)(group >> 8);
		}
		if (padding < 1) {
			out[written++] = (unsigned char)group;
		}
	}

	if (output_size != 0) {
		*output_size = written;
	}
	return 0;
}
