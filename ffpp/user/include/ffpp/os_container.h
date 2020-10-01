/*
 * os_container.h
 */

#ifndef OS_CONTAINER_H
#define OS_CONTAINER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * OS container related functions.
 *
 */

struct ffpp_cpu_rsc {
	uint64_t cfs_quota_us;
	uint64_t cfs_period_us;
};

/**
 * @brief ffpp_docker_update_cpu_rsc
 *
 * @param container_id
 * @param rsc
 *
 * @return
 */
int ffpp_docker_update_cpu_rsc(const char *container_id,
			       struct ffpp_cpu_rsc *rsc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !OS_CONTAINER_H */
