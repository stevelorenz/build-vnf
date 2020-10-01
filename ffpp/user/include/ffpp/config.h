/*
 * config.h
 */

/**
 * @file
 *
 * FFPP library configurations
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <rte_log.h>

/* U know, I love my Fanfan baby. */
#define FFBB_MAGIC 117

/* Logger defines */
#define RTE_LOGTYPE_FFPP RTE_LOGTYPE_USER1
#define RTE_LOGTYPE_TEST RTE_LOGTYPE_USER2

/* IO sleep */
#define FFPP_IO_SLEEP 1000 // ns

/* Number of mbuf pointers in one cache line. */
#define MBUFS_IN_ONE_CACHE_LINE 8

#endif /* !CONFIG_H */
