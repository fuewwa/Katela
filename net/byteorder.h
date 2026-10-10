#ifndef BYTEORDER_H
#define BYTEORDER_H

static inline unsigned short htons(unsigned short value)
{
	return (unsigned short)((value << 8) | (value >> 8));
}

static inline unsigned short ntohs(unsigned short value)
{
	return htons(value);
}

static inline unsigned int htonl(unsigned int value)
{
	return ((value & 0x000000FFu) << 24)
		 | ((value & 0x0000FF00u) << 8)
		 | ((value & 0x00FF0000u) >> 8)
		 | ((value & 0xFF000000u) >> 24);
}

static inline unsigned int ntohl(unsigned int value)
{
	return htonl(value);
}

static inline unsigned short be16_read(const unsigned char *bytes)
{
	return (unsigned short)(((unsigned int)bytes[0] << 8) | bytes[1]);
}

static inline unsigned int be32_read(const unsigned char *bytes)
{
	return ((unsigned int)bytes[0] << 24)
		 | ((unsigned int)bytes[1] << 16)
		 | ((unsigned int)bytes[2] << 8)
		 | (unsigned int)bytes[3];
}

static inline void be16_write(unsigned char *bytes, unsigned short value)
{
	bytes[0] = (unsigned char)(value >> 8);
	bytes[1] = (unsigned char)value;
}

static inline void be32_write(unsigned char *bytes, unsigned int value)
{
	bytes[0] = (unsigned char)(value >> 24);
	bytes[1] = (unsigned char)(value >> 16);
	bytes[2] = (unsigned char)(value >> 8);
	bytes[3] = (unsigned char)value;
}

#endif
