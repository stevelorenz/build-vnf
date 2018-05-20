/*
 * client.c
 * About : UDP client using raw sockets
 * TODO  : Support stdint.h
 */

#include <sys/socket.h>
#include <sys/time.h>

#include <linux/if_packet.h>

#include <net/ethernet.h>  /* the L2 protocols */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define FRAME_LEN 1500 /* Default MTU */
#define DATA_MAX_LEN 512  /* Maximal payload size */
#define DST_PORT 9999

/*
 * Generic checksum calculation function
 */
unsigned short csum(unsigned short *ptr,int nbytes)
{
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;

	return(answer);
}

int open_socket()
{
	/* AF_PACKET: the application should provide the full Ethernet frame
	 * ETH_P_ALL: all protocols are received */
	int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock == -1) {
		perror("Failed to create the raw packet socket\n");
		exit(1);
	}
	return sock;
}

/**
 * @brief Convert string formated MAC to bytes
 */
void mac_str2bytes(const char *mac_str, uint8_t * mac_bytes)
{
	int values[6], i;
	if( 6 == sscanf(mac_str, "%x:%x:%x:%x:%x:%x%*c",
				&values[0], &values[1], &values[2],
				&values[3], &values[4], &values[5] ) )
	{
		/* convert to uint8_t */
		for( i = 0; i < 6; ++i )
			mac_bytes[i] = (uint8_t) values[i];
	}

	else
	{
		perror("Invalid MAC address");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	/* TODO: Add args parser for these paras */
	const char *src_ip_addr = "127.0.0.1";
	const char *dst_ip_addr = "127.0.0.1";
	const char *src_mac_addr = "00:00:00:00:00:00";
	const char *dst_mac_addr = "00:00:00:00:00:ff";

	int dst_port = DST_PORT;

	/* Buffer for Ethernet frame */
	char fbuf[FRAME_LEN];
	memset(fbuf, 0 , FRAME_LEN);

	int send_sock = open_socket();

	/* Header pointers */
	struct ether_header *eth = (struct ether_header *)fbuf;
	struct iphdr *iph = (struct iphdr*)(fbuf + sizeof(struct ether_header));
	/* MARK: Assume here there are no header options */
	struct udphdr *udph = (struct udphdr *)(fbuf + sizeof(struct ether_header) + sizeof(struct iphdr));
	/* Payload pointer */
	char *data;
	data = fbuf + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct udphdr);
	/* Fill payload */
	strcpy(data , "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	/* Fill Ethernet header */
	mac_str2bytes(src_mac_addr, eth->ether_shost);
	mac_str2bytes(dst_mac_addr, eth->ether_dhost);
	eth->ether_type = ETHERTYPE_IP;

	/* Fill IP header */
	struct sockaddr_in src_in, dst_in;
	socklen_t slen;
	/* Src */
	src_in.sin_family = AF_INET;
	src_in.sin_addr.s_addr = inet_addr(src_ip_addr);
	src_in.sin_port = 0;
	/* Get source port via binding */
	bind(send_sock, (struct sockaddr *)&src_in, sizeof(src_in));
	slen = sizeof(src_in);
	getsockname(send_sock, (struct sockaddr *)&src_in, &slen);
	int src_port = ntohs(src_in.sin_port);
	printf("[INFO] Bind socket to IP:%s, port number:%d\n", src_ip_addr, src_port);
	/* DST */
	dst_in.sin_family = AF_INET;
	dst_in.sin_addr.s_addr = inet_addr(dst_ip_addr);
	dst_in.sin_port = dst_port;

	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(data);
	iph->id = htonl (54321); //Id of this packet
	iph->frag_off = 0;
	iph->ttl = 255;
	iph->protocol = IPPROTO_UDP;
	iph->check = 0;      // Set to 0 before calculating checksum
	iph->saddr = src_in.sin_addr.s_addr;
	iph->daddr = dst_in.sin_addr.s_addr;
	iph->check = csum((unsigned short *)(fbuf + sizeof(struct ether_header)), iph->tot_len);

	/* Fill UDP header */
	udph->source = htons(src_port);
	udph->dest = htons(dst_port);
	udph->len = htons(8 + strlen(data)); /* TCP header size 8 bytes */
	/* MARK UDP checksum is disabled if check == 0 */
	udph->check = 0;

	/* Send packet out */
	uint16_t len;
	uint16_t frame_len = sizeof(struct ether_header) + iph->tot_len;
	if ( ( len = sendto(send_sock, fbuf, frame_len, 0, (struct sockaddr*) &dst_in, sizeof(struct sockaddr)) ) < 0) {
		perror("Sendto failed");
	}
	else
	{
		printf("Send success, frame length: %d\n", len);
	}

	close(send_sock);
	return 0;
}
