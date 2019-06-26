/*
 * config.h
 */

/**
 * @file
 *
 * FastIO User library configurations
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <rte_log.h>

#define FFBB_MAGIC 117

/* Logger defines */
#define RTE_LOGTYPE_FASTIO_USER RTE_LOGTYPE_USER1
#define RTE_LOGTYPE_TEST RTE_LOGTYPE_USER2

/* IO sleep */
#define FASTIO_USER_IO_SLEEP 1000 // ns

#endif /* !CONFIG_H */
