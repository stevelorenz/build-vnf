/*
 * packet_processors.h
 */

#ifndef PACKET_PROCESSORS_H
#define PACKET_PROCESSORS_H

#include <stdint.h>

#include <rte_ether.h>

#include <ffpp/collections.h>

#ifdef __cplusplus
extern "C" {
#endif

void ffpp_pp_update_dl_dst(struct ffpp_mvec *vec,
			   const struct rte_ether_addr *dl_dst);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !PACKET_PROCESSORS_H */
