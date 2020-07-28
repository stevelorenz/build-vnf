/*
 * mvec.c
 */

#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_compat.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>

#include <ffpp/mvec.h>
#include <ffpp/utils.h>

struct mvec {
	uint16_t len; /**< Number of mbufs in the vector */
	uint16_t capacity; /**< Maximal number of mbufs in the vector */
	struct rte_mbuf **head; /**< Head pointer of a rte_mbuf array */
} __rte_cache_aligned;

struct mvec *mvec_new()
{
	struct mvec *vec =
		(struct mvec *)(rte_zmalloc("mvec", sizeof(struct mvec), 0));
	vec->len = 0;
	vec->capacity = MVEC_INIT_CAPACITY;
	vec->head = rte_zmalloc("mvec_head",
				sizeof(struct rte_mbuf *) * vec->capacity, 0);
	return vec;
}

__rte_always_inline struct rte_mbuf *mvec_at_index(struct mvec *vec, uint16_t n)
{
	if (n >= vec->len) {
		return NULL;
	}
	return *(vec->head + n);
}

__rte_always_inline uint16_t ffpp_mvec_len(struct mvec *vec)
{
	return vec->len;
}

void mvec_free(struct mvec *vec)
{
	rte_free(vec->head);
	rte_free(vec);
}

void mvec_free_mbufs_part(struct mvec *vec, uint16_t offset)
{
	uint16_t i = 0;

	for (i = offset; i < vec->len; ++i) {
		rte_pktmbuf_free(*(vec->head + i));
	}
	rte_free(vec);
}

void mvec_free_mbufs(struct mvec *vec)
{
	mvec_free_mbufs_part(vec, 0);
}

void print_mvec(struct mvec *vec)
{
	printf("Vector length: %u\n", vec->len);
	printf("Vector capacity: %u\n", vec->capacity);
	printf("Mbuf head offset: %d\n", rte_pktmbuf_headroom(*(vec->head)));
}

int mvec_datacmp(struct mvec *vec1, struct mvec *vec2)
{
	uint16_t len = 0;
	uint16_t i = 0;

	len = RTE_MIN(vec1->len, vec2->len);
	for (i = 0; i < len; ++i) {
		int ret = 0;
		ret = mbuf_datacmp(*(vec1->head + i), *(vec2->head + i));
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

void mvec_push(struct mvec *vec, uint16_t len)
{
	uint16_t i = 0;
	char *ret = NULL;
	struct rte_mbuf *mbuf;

	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		ret = rte_pktmbuf_prepend(mbuf, len);
		if (ret == NULL) {
			rte_panic("Header room of %d-th mbuf is not enough.\n",
				  i);
		}
	}
}

void mvec_push_u8(struct mvec *vec, uint8_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	mvec_push(vec, sizeof(value));
	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint8_t *), &value,
			   sizeof(value));
	}
}

void mvec_push_u16(struct mvec *vec, uint16_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	mvec_push(vec, sizeof(value));
	value = rte_cpu_to_be_16(value);
	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint16_t *), &value,
			   sizeof(value));
	}
}

void mvec_push_u32(struct mvec *vec, uint32_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	mvec_push(vec, sizeof(value));
	value = rte_cpu_to_be_32(value);
	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint32_t *), &value,
			   sizeof(value));
	}
}

void mvec_push_u64(struct mvec *vec, uint64_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	mvec_push(vec, sizeof(value));
	value = rte_cpu_to_be_64(value);
	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint64_t *), &value,
			   sizeof(value));
	}
}

void mvec_pull(struct mvec *vec, uint16_t len)
{
	uint16_t i;
	char *ret = NULL;
	struct rte_mbuf *mbuf;

	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		ret = rte_pktmbuf_adj(mbuf, len);
		if (ret == NULL) {
			rte_panic("Data room of %d-th mbuf is not enough.\n",
				  i);
		}
	}
}

void mvec_pull_u8(struct mvec *vec, uint8_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		*(values + i) = *(rte_pktmbuf_mtod(mbuf, uint8_t *));
	}
	mvec_pull(vec, sizeof(uint8_t));
}

void mvec_pull_u16(struct mvec *vec, uint16_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(values + i, rte_pktmbuf_mtod(mbuf, uint16_t *),
			   sizeof(uint16_t));
		*(values + i) = rte_be_to_cpu_16(*(values + i));
	}
	mvec_pull(vec, sizeof(uint16_t));
}

void mvec_pull_u32(struct mvec *vec, uint32_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(values + i, rte_pktmbuf_mtod(mbuf, uint32_t *),
			   sizeof(uint32_t));
		*(values + i) = rte_be_to_cpu_32(*(values + i));
	}
	mvec_pull(vec, sizeof(uint32_t));
}

void mvec_pull_u64(struct mvec *vec, uint64_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	MVEC_FOREACH_MBUF(i, mbuf, vec)
	{
		rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(values + i, rte_pktmbuf_mtod(mbuf, uint64_t *),
			   sizeof(uint64_t));
		*(values + i) = rte_be_to_cpu_64(*(values + i));
	}
	mvec_pull(vec, sizeof(uint64_t));
}
