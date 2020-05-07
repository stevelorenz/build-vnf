/*
 * main.c
 *
 * About: Network coding UDP segments with normal INET socket in layer 4
 *
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 1024

#define RECV_IFCE_IP "10.0.0.11"
#define SEND_IFCE_IP "10.0.0.12"
#define DST_IFCE_IP "10.0.0.14"

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char **argv)
{
	int recv_sock; /* socket */
	int send_sock;
	int portno; /* port to listen on */
	unsigned int clt_addr_len; /* byte size of client's address */
	struct sockaddr_in srv_recv_addr; /* server's addr */
	struct in_addr recv_ip;
	struct sockaddr_in srv_send_addr;
	struct in_addr send_ip;
	struct sockaddr_in clt_addr; /* client addr */
	struct sockaddr_in dst_addr;
	struct in_addr dst_ip;
	char buf[BUFSIZE]; /* message buf */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */
	unsigned long recv_num = 0; /* number of received segments */

	/*
         * check command line arguments
         */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (recv_sock < 0)
		error("ERROR opening ingress socket");

	send_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (send_sock < 0) {
		error("ERROR opening egress socket");
	}

	/* setsockopt: Handy debugging trick that lets
         * us rerun the server immediately after we kill it;
         * otherwise we have to wait about 20 secs.
         * Eliminates "ERROR on binding: Address already in use" error.
         */
	optval = 1;
	setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
		   sizeof(int));
	setsockopt(send_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
		   sizeof(int));

	bzero((char *)&srv_recv_addr, sizeof(srv_recv_addr));
	srv_recv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, RECV_IFCE_IP, &recv_ip);
	srv_recv_addr.sin_addr.s_addr = recv_ip.s_addr;
	srv_recv_addr.sin_port = htons((unsigned short)portno);

	bzero((char *)&srv_send_addr, sizeof(srv_send_addr));
	srv_send_addr.sin_family = AF_INET;
	inet_pton(AF_INET, SEND_IFCE_IP, &send_ip);
	srv_send_addr.sin_addr.s_addr = send_ip.s_addr;
	srv_send_addr.sin_port = htons((unsigned short)portno);

	bzero((char *)&dst_addr, sizeof(dst_addr));
	dst_addr.sin_family = AF_INET;
	inet_pton(AF_INET, DST_IFCE_IP, &dst_ip);
	dst_addr.sin_addr.s_addr = dst_ip.s_addr;
	dst_addr.sin_port = htons((unsigned short)portno);

	if (bind(recv_sock, (struct sockaddr *)&srv_recv_addr,
		 sizeof(srv_recv_addr)) < 0)
		error("ERROR on binding recv socket.");

	if (bind(send_sock, (struct sockaddr *)&srv_send_addr,
		 sizeof(srv_send_addr)) < 0) {
		error("ERROR on binding send socket.");
	}

	/*
         * main loop:
         */
	printf("Listen on port: %d...\n", portno);
	clt_addr_len = sizeof(clt_addr);
	while (1) {
		n = recvfrom(recv_sock, buf, BUFSIZE, 0,
			     (struct sockaddr *)&clt_addr, &clt_addr_len);
		if (n < 0)
			error("ERROR in recvfrom");

		recv_num += 1;
		printf("Already recv %lu UDP segments.\n", recv_num);

		/* Simply forwarding */
		sendto(send_sock, buf, n, 0, (struct sockaddr *)&dst_addr,
		       sizeof(dst_addr));
	}
}
