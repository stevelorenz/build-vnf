/*
 * ffpp_mvec.c
 */

#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_compat.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>

#include <ffpp/mvec.h>
#include <ffpp/utils.h>

int ffpp_mvec_init(struct ffpp_mvec *vec, uint16_t size)
{
	vec->len = 0;
	vec->capacity = size;
	vec->socket_id = rte_socket_id();
	vec->head = rte_malloc_socket("ffpp_mvec",
				      sizeof(struct rte_mbuf *) * vec->capacity,
				      64, vec->socket_id);
	if (vec->head == NULL) {
		return -1;
	}
	return 0;
}

int ffpp_mvec_set_mbufs(struct ffpp_mvec *vec, struct rte_mbuf **buf,
			uint16_t size)
{
	if (size > vec->capacity) {
		return -1;
	}
	vec->len = size;
	uint16_t i = 0;
	for (i = 0; i < size; ++i) {
		*(vec->head + i) = buf[i];
	}
	return 0;
}

void ffpp_mvec_free(struct ffpp_mvec *vec)
{
	rte_free(vec->head);
}

__rte_always_inline struct rte_mbuf *ffpp_mvec_at_index(struct ffpp_mvec *vec,
							uint16_t n)
{
	if (n >= vec->len) {
		return NULL;
	}
	return *(vec->head + n);
}

__rte_always_inline uint16_t ffpp_mvec_len(struct ffpp_mvec *vec)
{
	return vec->len;
}

void ffpp_mvec_free_mbufs_part(struct ffpp_mvec *vec, uint16_t offset)
{
	uint16_t i = 0;

	for (i = offset; i < vec->len; ++i) {
		rte_pktmbuf_free(*(vec->head + i));
	}
	rte_free(vec);
}

void ffpp_mvec_free_mbufs(struct ffpp_mvec *vec)
{
	ffpp_mvec_free_mbufs_part(vec, 0);
}

void ffpp_mvec_print(const struct ffpp_mvec *vec)
{
	printf("Vector length: %u\n", vec->len);
	printf("Vector capacity: %u\n", vec->capacity);
	printf("Mbuf head offset: %d\n", rte_pktmbuf_headroom(*(vec->head)));
}

int ffpp_mvec_datacmp(struct ffpp_mvec *vec1, struct ffpp_mvec *vec2)
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

void ffpp_mvec_push(struct ffpp_mvec *vec, uint16_t len)
{
	uint16_t i = 0;
	char *ret = NULL;
	struct rte_mbuf *mbuf;

	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		ret = rte_pktmbuf_prepend(mbuf, len);
		if (ret == NULL) {
			rte_panic("Header room of %d-th mbuf is not enough.\n",
				  i);
		}
	}
}

void ffpp_mvec_push_u8(struct ffpp_mvec *vec, uint8_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	ffpp_mvec_push(vec, sizeof(value));
	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint8_t *), &value,
			   sizeof(value));
	}
}

void ffpp_mvec_push_u16(struct ffpp_mvec *vec, uint16_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	ffpp_mvec_push(vec, sizeof(value));
	value = rte_cpu_to_be_16(value);
	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint16_t *), &value,
			   sizeof(value));
	}
}

void ffpp_mvec_push_u32(struct ffpp_mvec *vec, uint32_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	ffpp_mvec_push(vec, sizeof(value));
	value = rte_cpu_to_be_32(value);
	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint32_t *), &value,
			   sizeof(value));
	}
}

void ffpp_mvec_push_u64(struct ffpp_mvec *vec, uint64_t value)
{
	uint16_t i = 0;
	struct rte_mbuf *mbuf;

	ffpp_mvec_push(vec, sizeof(value));
	value = rte_cpu_to_be_64(value);
	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		rte_prefetch_non_temporal(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(rte_pktmbuf_mtod(mbuf, uint64_t *), &value,
			   sizeof(value));
	}
}

void ffpp_mvec_pull(struct ffpp_mvec *vec, uint16_t len)
{
	uint16_t i;
	char *ret = NULL;
	struct rte_mbuf *mbuf;

	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		ret = rte_pktmbuf_adj(mbuf, len);
		if (ret == NULL) {
			rte_panic("Data room of %d-th mbuf is not enough.\n",
				  i);
		}
	}
}

void ffpp_mvec_pull_u8(struct ffpp_mvec *vec, uint8_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		*(values + i) = *(rte_pktmbuf_mtod(mbuf, uint8_t *));
	}
	ffpp_mvec_pull(vec, sizeof(uint8_t));
}

void ffpp_mvec_pull_u16(struct ffpp_mvec *vec, uint16_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(values + i, rte_pktmbuf_mtod(mbuf, uint16_t *),
			   sizeof(uint16_t));
		*(values + i) = rte_be_to_cpu_16(*(values + i));
	}
	ffpp_mvec_pull(vec, sizeof(uint16_t));
}

void ffpp_mvec_pull_u32(struct ffpp_mvec *vec, uint32_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(values + i, rte_pktmbuf_mtod(mbuf, uint32_t *),
			   sizeof(uint32_t));
		*(values + i) = rte_be_to_cpu_32(*(values + i));
	}
	ffpp_mvec_pull(vec, sizeof(uint32_t));
}

void ffpp_mvec_pull_u64(struct ffpp_mvec *vec, uint64_t *values)
{
	uint16_t i;
	struct rte_mbuf *mbuf;

	FFPP_MVEC_FOREACH(vec, i, mbuf)
	{
		rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));
		rte_memcpy(values + i, rte_pktmbuf_mtod(mbuf, uint64_t *),
			   sizeof(uint64_t));
		*(values + i) = rte_be_to_cpu_64(*(values + i));
	}
	ffpp_mvec_pull(vec, sizeof(uint64_t));
}
