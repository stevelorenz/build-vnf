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

/*
 * error - wrapper for perror
 */
void error(char* msg)
{
        perror(msg);
        exit(1);
}

int main(int argc, char** argv)
{
        int sockfd;                    /* socket */
        int portno;                    /* port to listen on */
        unsigned int clientlen;        /* byte size of client's address */
        struct sockaddr_in serveraddr; /* server's addr */
        struct sockaddr_in clientaddr; /* client addr */
        char* buf;                     /* message buf */
        int optval;                    /* flag value for setsockopt */
        int n;                         /* message byte size */

        unsigned long recv_num = 0;

        /*
         * check command line arguments
         */
        if (argc != 2) {
                fprintf(stderr, "usage: %s <port>\n", argv[0]);
                exit(1);
        }
        portno = atoi(argv[1]);

        /*
         * socket: create the parent socket
         */
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
                error("ERROR opening socket");

        /* setsockopt: Handy debugging trick that lets
         * us rerun the server immediately after we kill it;
         * otherwise we have to wait about 20 secs.
         * Eliminates "ERROR on binding: Address already in use" error.
         */
        optval = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
            sizeof(int));

        /*
         * build the server's Internet address
         */
        bzero((char*)&serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serveraddr.sin_port = htons((unsigned short)portno);

        /*
         * bind: associate the parent socket with a port
         */
        if (bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
                error("ERROR on binding");

        /*
         * main loop: wait for a datagram, then echo it
         */
        printf("Listen on port: %d...\n", portno);
        clientlen = sizeof(clientaddr);
        while (1) {

                /*
                 * recvfrom: receive a UDP datagram from a client
                 */
                buf = malloc(BUFSIZE);
                n = recvfrom(sockfd, buf, BUFSIZE, 0,
                    (struct sockaddr*)&clientaddr, &clientlen);
                if (n < 0)
                        error("ERROR in recvfrom");

                recv_num += 1;
                printf("Already recv %lu UDP segments.\n", recv_num);
        }
}
