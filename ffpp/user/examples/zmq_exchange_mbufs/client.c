/*
 * client.c
 *
 * Client for fast-path data processing.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <rte_eal.h>
#include <rte_cycles.h>

#include <ffpp/munf.h>
#include <zmq.h>

int main(int argc, char *argv[])
{
	int ret;
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
	}
	argc -= ret;
	argv += ret;

	rte_eal_cleanup();
	return 0;
}
