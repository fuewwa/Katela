#ifndef ICMP_H
#define ICMP_H

#define ICMP_HEADER_SIZE 8

#define ICMP_ECHO_REPLY 0
#define ICMP_DEST_UNREACHABLE 3
#define ICMP_ECHO_REQUEST 8
#define ICMP_TIME_EXCEEDED 11

#define ICMP_CODE_NET_UNREACHABLE 0
#define ICMP_CODE_HOST_UNREACHABLE 1
#define ICMP_CODE_PROTOCOL_UNREACHABLE 2
#define ICMP_CODE_PORT_UNREACHABLE 3

struct icmp_message {
	unsigned char type;
	unsigned char code;
	unsigned short id;
	unsigned short sequence;
	const unsigned char *payload;
	unsigned int payload_size;
};

int icmp_parse(const void *data, unsigned int size, struct icmp_message *message);
unsigned int icmp_build(void *data, unsigned int capacity, unsigned char type, unsigned char code,
		unsigned short id, unsigned short sequence, const void *payload, unsigned int payload_size);
unsigned int icmp_echo_reply(void *data, unsigned int capacity, const struct icmp_message *request);
unsigned int icmp_build_error(void *data, unsigned int capacity, unsigned char type, unsigned char code,
		const void *original, unsigned int original_size);

#endif
