#include "../../include/net.h"
#include "../../include/irq.h"
#include "../../include/mem.h"
#include "../../include/scheduler.h"
#include "../drivers/rtl8139.h"
#include "../../net/arp.h"
#include "../../net/dns.h"
#include "../../net/ethernet.h"
#include "../../net/icmp.h"
#include "../../net/ipv4.h"
#include "../../net/udp.h"

#define PING_ID 0x4B54
#define PING_PAYLOAD_SIZE 32
#define ARP_ATTEMPTS 3
#define ARP_WAIT_MS 1000
#define DNS_ATTEMPTS 3
#define DNS_PORT_BASE 49152
#define DNS_PORT_COUNT 16384

struct resolve_state {
	const struct ipv4_addr *ip;
	struct mac_addr mac;
};

static struct mac_addr own_mac;
static struct ipv4_addr own_ip = {{10, 0, 2, 15}};
static struct ipv4_addr gateway = {{10, 0, 2, 2}};
static struct ipv4_addr dns_server = {{10, 0, 2, 3}};
static struct ipv4_addr netmask = {{255, 255, 255, 0}};
static struct arp_table table;
static struct wait_queue events;
static volatile int active;

static unsigned char rx_frame[ETHERNET_FRAME_MAX];
static unsigned char reply_icmp[ETHERNET_MTU];
static unsigned char reply_ip[ETHERNET_MTU];
static unsigned char reply_frame[ETHERNET_FRAME_MAX];

static struct ipv4_addr echo_target;
static unsigned short echo_sequence;
static volatile int echo_waiting;
static volatile int echo_received;
static volatile unsigned int echo_time;

static unsigned char dns_query[DNS_MESSAGE_MAX];
static unsigned char dns_datagram[UDP_HEADER_SIZE + DNS_MESSAGE_MAX];
static unsigned char dns_packet[IPV4_HEADER_SIZE + UDP_HEADER_SIZE + DNS_MESSAGE_MAX];
static unsigned char dns_frame[ETHERNET_HEADER_SIZE + IPV4_HEADER_SIZE + UDP_HEADER_SIZE + DNS_MESSAGE_MAX];
static char dns_name[DNS_NAME_MAX + 1];
static struct ipv4_addr dns_address;
static unsigned short dns_id;
static unsigned short dns_local_port;
static volatile int dns_waiting;
static volatile int dns_done;
static volatile int dns_status;

static int send_ipv4(const struct mac_addr *destination, const struct ipv4_addr *target,
		unsigned char protocol, const void *payload, unsigned int size,
		unsigned char *packet_buffer, unsigned int packet_capacity,
		unsigned char *frame_buffer, unsigned int frame_capacity)
{
	struct ipv4_header ip;
	struct ethernet_header header;
	unsigned int packet_size;
	unsigned int frame_size;

	ipv4_header_init(&ip, &own_ip, target, protocol);
	packet_size = ipv4_packet_build(packet_buffer, packet_capacity, &ip, payload, size);
	if (packet_size == 0) {
		return -1;
	}

	header.destination = *destination;
	header.source = own_mac;
	header.type = ETHERTYPE_IPV4;

	frame_size = ethernet_build(frame_buffer, frame_capacity, &header, packet_buffer, packet_size);
	if (frame_size == 0) {
		return -1;
	}

	return rtl8139_send(frame_buffer, frame_size) < 0 ? -1 : 0;
}

static int table_lookup(const struct ipv4_addr *ip, struct mac_addr *mac)
{
	unsigned int flags = irq_save();
	int result = arp_table_lookup(&table, ip, mac, uptime_ms());

	irq_restore(flags);
	return result;
}

static int resolved(void *arg)
{
	struct resolve_state *state = (struct resolve_state *)arg;

	return arp_table_lookup(&table, state->ip, &state->mac, uptime_ms()) == 0;
}

static int resolve(const struct ipv4_addr *ip, struct mac_addr *mac)
{
	struct resolve_state state;
	unsigned char frame[ETHERNET_FRAME_MIN];
	struct arp_packet request;
	unsigned int attempt;
	unsigned int size;

	if (table_lookup(ip, mac) == 0) {
		return 0;
	}

	state.ip = ip;

	for (attempt = 0; attempt < ARP_ATTEMPTS; attempt++) {
		arp_make_request(&request, &own_mac, &own_ip, ip);
		size = arp_frame(frame, sizeof(frame), &request);
		if (size == 0 || rtl8139_send(frame, size) < 0) {
			return -1;
		}

		if (wait_queue_wait_until(&events, resolved, &state, ARP_WAIT_MS) == WAIT_OK) {
			*mac = state.mac;
			return 0;
		}
	}

	return -1;
}

static void handle_arp(const unsigned char *data, unsigned int size)
{
	struct arp_packet packet;
	struct arp_packet reply;
	unsigned int flags;
	unsigned int length;
	int answer;

	if (arp_parse(data, size, &packet) != 0) {
		return;
	}

	flags = irq_save();
	answer = arp_handle(&table, &packet, &own_mac, &own_ip, uptime_ms(), &reply);
	irq_restore(flags);

	wait_queue_wake_all(&events);

	if (answer) {
		length = arp_frame(reply_frame, sizeof(reply_frame), &reply);
		if (length != 0) {
			rtl8139_send(reply_frame, length);
		}
	}
}

static void handle_icmp(const struct ethernet_header *frame_header, const struct ipv4_header *ip,
		const unsigned char *data, unsigned int size)
{
	struct icmp_message message;
	unsigned int length;

	if (icmp_parse(data, size, &message) != 0) {
		return;
	}

	if (message.type == ICMP_ECHO_REQUEST) {
		length = icmp_echo_reply(reply_icmp, sizeof(reply_icmp), &message);
		if (length != 0) {
			send_ipv4(&frame_header->source, &ip->source, IPV4_PROTOCOL_ICMP, reply_icmp, length,
					reply_ip, sizeof(reply_ip), reply_frame, sizeof(reply_frame));
		}
		return;
	}

	if (message.type == ICMP_ECHO_REPLY && echo_waiting
		&& message.id == PING_ID && message.sequence == echo_sequence
		&& ipv4_equal(&ip->source, &echo_target)) {
		echo_time = uptime_ms();
		echo_received = 1;
		wait_queue_wake_all(&events);
	}
}

static void handle_udp(const struct ipv4_header *ip, const unsigned char *data, unsigned int size)
{
	struct udp_header udp;
	const unsigned char *payload;
	unsigned int payload_size;
	struct ipv4_addr address;
	int result;

	if (udp_parse(data, size, &ip->source, &ip->destination, &udp, &payload, &payload_size) != 0) {
		return;
	}

	if (!dns_waiting || udp.destination_port != dns_local_port || udp.source_port != DNS_PORT
		|| !ipv4_equal(&ip->source, &dns_server)) {
		return;
	}

	result = dns_response_parse(payload, payload_size, dns_id, dns_name, &address);
	if (result == DNS_ERR_MALFORMED || result == DNS_ERR_MISMATCH) {
		return;
	}

	dns_address = address;
	dns_status = result;
	dns_done = 1;
	wait_queue_wake_all(&events);
}

static void handle_ipv4(const struct ethernet_header *frame_header, const unsigned char *data, unsigned int size)
{
	struct ipv4_header ip;
	const unsigned char *payload;
	unsigned int payload_size;

	if (ipv4_packet_parse(data, size, &ip, &payload, &payload_size) != 0) {
		return;
	}

	if (!ipv4_equal(&ip.destination, &own_ip)) {
		return;
	}

	if (ip.protocol == IPV4_PROTOCOL_ICMP) {
		handle_icmp(frame_header, &ip, payload, payload_size);
	} else if (ip.protocol == IPV4_PROTOCOL_UDP) {
		handle_udp(&ip, payload, payload_size);
	}
}

static void handle_frame(const unsigned char *frame, unsigned int size)
{
	struct ethernet_header header;
	const unsigned char *payload;
	unsigned int payload_size;

	if (ethernet_parse(frame, size, &header, &payload, &payload_size) != 0) {
		return;
	}

	if (!ethernet_for_us(&header, &own_mac)) {
		return;
	}

	if (header.type == ETHERTYPE_ARP) {
		handle_arp(payload, payload_size);
	} else if (header.type == ETHERTYPE_IPV4) {
		handle_ipv4(&header, payload, payload_size);
	}
}

static void receive_task(void)
{
	while (1) {
		int size = rtl8139_receive(rx_frame, sizeof(rx_frame), WAIT_FOREVER);

		if (size > 0) {
			handle_frame(rx_frame, (unsigned int)size);
		}
	}
}

static int dns_ready(void *arg)
{
	(void)arg;
	return dns_done;
}

static unsigned int entropy(void)
{
	unsigned int low;
	unsigned int high;

	asm volatile("rdtsc" : "=a"(low), "=d"(high));
	(void)high;

	return low ^ (low >> 16);
}

static int echo_ready(void *arg)
{
	(void)arg;
	return echo_received;
}

int net_init(void)
{
	int result;

	if (active) {
		return NET_OK;
	}

	result = rtl8139_init();
	if (result != RTL8139_OK) {
		return NET_ERR_NO_DEVICE;
	}

	own_mac = *rtl8139_mac();
	arp_table_init(&table);
	wait_queue_init(&events);

	if (task_create(receive_task, "net") < 0) {
		return NET_ERR_NO_DEVICE;
	}

	active = 1;
	return NET_OK;
}

int net_ready(void)
{
	return active;
}

int net_ping(const struct ipv4_addr *target, unsigned int timeout_ms, unsigned int *elapsed_ms)
{
	unsigned char payload[ICMP_HEADER_SIZE + PING_PAYLOAD_SIZE];
	unsigned char data[PING_PAYLOAD_SIZE];
	unsigned char packet[IPV4_HEADER_SIZE + ICMP_HEADER_SIZE + PING_PAYLOAD_SIZE];
	unsigned char frame[ETHERNET_HEADER_SIZE + IPV4_HEADER_SIZE + ICMP_HEADER_SIZE + PING_PAYLOAD_SIZE];
	const struct ipv4_addr *next_hop;
	struct mac_addr destination;
	unsigned int length;
	unsigned int start;
	unsigned int i;

	if (!active) {
		return NET_ERR_NO_DEVICE;
	}

	next_hop = ipv4_in_subnet(target, &own_ip, &netmask) ? target : &gateway;

	if (resolve(next_hop, &destination) != 0) {
		return NET_ERR_NO_ROUTE;
	}

	for (i = 0; i < PING_PAYLOAD_SIZE; i++) {
		data[i] = (unsigned char)('a' + (i % 26));
	}

	echo_sequence++;
	echo_target = *target;
	echo_received = 0;
	echo_waiting = 1;

	length = icmp_build(payload, sizeof(payload), ICMP_ECHO_REQUEST, 0, PING_ID, echo_sequence, data, sizeof(data));
	start = uptime_ms();

	if (length == 0 || send_ipv4(&destination, target, IPV4_PROTOCOL_ICMP, payload, length,
			packet, sizeof(packet), frame, sizeof(frame)) != 0) {
		echo_waiting = 0;
		return NET_ERR_SEND;
	}

	if (wait_queue_wait_until(&events, echo_ready, 0, timeout_ms) != WAIT_OK) {
		echo_waiting = 0;
		return NET_ERR_TIMEOUT;
	}

	echo_waiting = 0;

	if (elapsed_ms) {
		*elapsed_ms = echo_time - start;
	}

	return NET_OK;
}

int net_resolve(const char *name, struct ipv4_addr *address, unsigned int timeout_ms)
{
	struct mac_addr destination;
	const struct ipv4_addr *next_hop;
	unsigned int query_size;
	unsigned int datagram_size;
	unsigned int attempt;
	unsigned int i;

	if (!active) {
		return NET_ERR_NO_DEVICE;
	}

	for (i = 0; name[i] != 0; i++) {
		if (i >= DNS_NAME_MAX) {
			return NET_ERR_NAME;
		}
		dns_name[i] = name[i];
	}
	dns_name[i] = 0;

	if (dns_query_build(dns_query, sizeof(dns_query), 0, dns_name) == 0) {
		return NET_ERR_NAME;
	}

	next_hop = ipv4_in_subnet(&dns_server, &own_ip, &netmask) ? &dns_server : &gateway;

	if (resolve(next_hop, &destination) != 0) {
		return NET_ERR_NO_ROUTE;
	}

	for (attempt = 0; attempt < DNS_ATTEMPTS; attempt++) {
		unsigned int random = entropy();

		dns_id = (unsigned short)random;
		dns_local_port = (unsigned short)(DNS_PORT_BASE + (random >> 3) % DNS_PORT_COUNT);
		dns_done = 0;
		dns_status = 0;

		query_size = dns_query_build(dns_query, sizeof(dns_query), dns_id, dns_name);
		datagram_size = udp_build(dns_datagram, sizeof(dns_datagram), &own_ip, &dns_server,
				dns_local_port, DNS_PORT, dns_query, query_size);
		if (query_size == 0 || datagram_size == 0) {
			return NET_ERR_NAME;
		}

		dns_waiting = 1;

		if (send_ipv4(&destination, &dns_server, IPV4_PROTOCOL_UDP, dns_datagram, datagram_size,
				dns_packet, sizeof(dns_packet), dns_frame, sizeof(dns_frame)) != 0) {
			dns_waiting = 0;
			return NET_ERR_SEND;
		}

		if (wait_queue_wait_until(&events, dns_ready, 0, timeout_ms) == WAIT_OK) {
			dns_waiting = 0;

			if (dns_status == 0) {
				*address = dns_address;
				return NET_OK;
			}

			return dns_status == DNS_ERR_NXDOMAIN ? NET_ERR_NXDOMAIN : NET_ERR_DNS;
		}

		dns_waiting = 0;
	}

	return NET_ERR_TIMEOUT;
}

const struct mac_addr *net_mac(void)
{
	return &own_mac;
}

const struct ipv4_addr *net_ip(void)
{
	return &own_ip;
}

const struct ipv4_addr *net_gateway(void)
{
	return &gateway;
}

const struct ipv4_addr *net_dns(void)
{
	return &dns_server;
}
