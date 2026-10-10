#ifndef ADDR_H
#define ADDR_H

#define MAC_SIZE 6
#define MAC_STRING_SIZE 18
#define IPV4_SIZE 4
#define IPV4_STRING_SIZE 16

struct mac_addr {
	unsigned char bytes[MAC_SIZE];
};

struct ipv4_addr {
	unsigned char bytes[IPV4_SIZE];
};

int mac_parse(const char *text, struct mac_addr *addr);
void mac_format(const struct mac_addr *addr, char *out);
int mac_equal(const struct mac_addr *a, const struct mac_addr *b);
int mac_is_zero(const struct mac_addr *addr);
int mac_is_broadcast(const struct mac_addr *addr);
int mac_is_multicast(const struct mac_addr *addr);

int ipv4_parse(const char *text, struct ipv4_addr *addr);
void ipv4_format(const struct ipv4_addr *addr, char *out);
int ipv4_equal(const struct ipv4_addr *a, const struct ipv4_addr *b);
unsigned int ipv4_to_u32(const struct ipv4_addr *addr);
void ipv4_from_u32(struct ipv4_addr *addr, unsigned int value);

#endif
