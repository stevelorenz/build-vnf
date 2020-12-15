/*
 * utils.h
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#include <rte_mbuf.h>
#include <rte_mempool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * Utility functions
 *
 */

/*********************
 *  Mbuf Operations  *
 *********************/

/**
 * mbuf_deep_copy() -  Make a deep copy of a mbuf. The returned copied mbuf is
 * allocated from the mbuf_pool, the head room is adjusted, the data room is
 * copied from the given mbuf.
 *
 * @param mbuf_pool
 * @param m
 *
 * @return
 */
struct rte_mbuf *mbuf_deep_copy(struct rte_mempool *mbuf_pool,
				struct rte_mbuf *m);

/**
 * mbuf_datacmp() - Compare two mbufs' data room
 *
 * @param m1
 * @param m2
 *
 * @return
 */
int mbuf_datacmp(struct rte_mbuf *m1, struct rte_mbuf *m2);

/******************
 *  Cycles/Timer  *
 ******************/

/**
 * get_delay_tsc_ms() - Get delay based on TSC counts
 *
 * @param tsc_cnt
 *
 * @return
 */
double get_delay_tsc_ms(uint64_t tsc_cnt);

void save_double_list_csv(char *path, double *list, size_t n);
void save_u64_list_csv(char *path, uint64_t *list, size_t n);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !UTILS_H */
