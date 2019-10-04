/*
 * utils.c
 */

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_prefetch.h>

#include "utils.h"

/* TODO: <29-06-19, Zuo> Add support for multi-segment mbuf and header
 * adjustment */
struct rte_mbuf *mbuf_deep_copy(struct rte_mempool *mbuf_pool,
				struct rte_mbuf *m)
{
	if (m->nb_segs > 1) {
		rte_exit(EXIT_FAILURE,
			 "Deep copy doest not support scattered segments.\n");
	}
	struct rte_mbuf *m_copy;
	m_copy = rte_pktmbuf_alloc(mbuf_pool);
	m_copy->data_len = m->data_len;
	m_copy->pkt_len = m->pkt_len;
	if (rte_pktmbuf_headroom(m) != RTE_PKTMBUF_HEADROOM) {
		rte_exit(EXIT_FAILURE, "mbuf's header room is not default.\n");
	}
	rte_memcpy(rte_pktmbuf_mtod(m_copy, uint8_t *),
		   rte_pktmbuf_mtod(m, uint8_t *), m->data_len);
	return m_copy;
}

int mbuf_datacmp(struct rte_mbuf *m1, struct rte_mbuf *m2)
{
	uint8_t *pt_m1 = NULL;
	uint8_t *pt_m2 = NULL;
	uint16_t len = 0;
	size_t i = 0;
	len = RTE_MIN(m1->data_len, m2->data_len);
	pt_m1 = rte_pktmbuf_mtod(m1, uint8_t *);
	pt_m2 = rte_pktmbuf_mtod(m2, uint8_t *);
	rte_prefetch_non_temporal((void *)pt_m1);
	rte_prefetch_non_temporal((void *)pt_m2);
	for (i = 0; i < len; ++i) {
		if (*pt_m1 < *pt_m2) {
			return -1;
		} else if (*pt_m1 > *pt_m2) {
			return 1;
		}
	}
	return 0;
}

double get_delay_tsc_ms(uint64_t tsc_cnt)
{
	double delay = 0;
	delay = 1000.0 * ((1.0 / rte_get_tsc_hz()) * (tsc_cnt));
	return delay;
}

void save_double_list_csv(char *path, double *list, size_t n)
{
	size_t i;
	FILE *fd;

	fd = fopen(path, "a+");
	if (fd == NULL) {
		perror("Can not open the CSV file!");
	}
	for (i = 0; i < n; ++i) {
		fprintf(fd, "%.8f,", list[i]);
	}
	fprintf(fd, "\n");
	fclose(fd);
}

void save_u64_list_csv(char *path, uint64_t *list, size_t n)
{
	size_t i;
	FILE *fd;

	fd = fopen(path, "a+");
	if (fd == NULL) {
		perror("Can not open the CSV file!");
	}
	for (i = 0; i < n; ++i) {
		fprintf(fd, "%lu,", list[i]);
	}
	fprintf(fd, "\n");
	fclose(fd);
}
