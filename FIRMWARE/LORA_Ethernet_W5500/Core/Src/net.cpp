#include "net.h"

uint8_t ipaddr[4] = IP_ADDR;

uint8_t ipgate[4] = IP_GATE;

uint8_t ipmask[4] = IP_MASK;

uint16_t local_port = LOCAL_PORT;

char str1[60] = {0};

void packet_receive(void) {
	w5500_packetReceive();
}

void net_poll(void) {
	packet_receive();
}

void net_init(void) {
	w5500_init();
}
