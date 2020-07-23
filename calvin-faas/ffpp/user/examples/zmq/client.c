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

#include <zmq.h>
#include <jansson.h>

int main(int argc, char *argv[])
{
	int ret;
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
	}
	argc -= ret;
	argv += ret;

	json_t *root = json_object();
	char *req_msg;

	ret = json_object_set_new(root, "action", json_string("off"));
	req_msg = json_dumps(root, 0);
	printf("Request message: %s\n", req_msg);

	void *context = zmq_ctx_new();
	void *requester = zmq_socket(context, ZMQ_REQ);
	char buf[10];
	uint64_t prev_tsc = 0, diff_tsc;
	float dur = 0.0;
	uint16_t i;

	for (i = 0; i < 5; ++i) {
		prev_tsc = rte_rdtsc();
		zmq_connect(requester, "tcp://127.0.0.1:9999");
		zmq_send(requester, req_msg, strlen(req_msg), 0);
		zmq_recv(requester, buf, 10, 0);
		diff_tsc = rte_rdtsc() - prev_tsc;
		dur = ((float)(diff_tsc) / rte_get_timer_hz()) * 1000;
		printf("The cost of one REQ-REP. Cycles: %lu, time: %fms\n",
		       diff_tsc, dur);
		printf("Response message: %s\n", buf);
	}

	zmq_close(requester);
	zmq_ctx_destroy(context);
	free(req_msg);
	// Decrease reference counting to trigger free.
	json_decref(root);

	return 0;
}
