#ifndef DNS_H
#define DNS_H

#include "addr.h"

#define DNS_PORT 53
#define DNS_NAME_MAX 253
#define DNS_MESSAGE_MAX 512

#define DNS_ERR_MALFORMED -1
#define DNS_ERR_NAME -2
#define DNS_ERR_MISMATCH -3
#define DNS_ERR_NXDOMAIN -4
#define DNS_ERR_SERVER -5
#define DNS_ERR_NO_ANSWER -6

unsigned int dns_query_build(void *data, unsigned int capacity, unsigned short id, const char *name);
int dns_response_parse(const void *data, unsigned int size, unsigned short id, const char *name,
		struct ipv4_addr *address);

#endif
