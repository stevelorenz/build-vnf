/*
 * kni.h
 * About: Kernel NIC interface
 */

#ifndef KNI_H
#define KNI_H

#include <stdint.h>
#include <stdlib.h>

#include <rte_atomic.h>
#include <rte_common.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_per_lcore.h>

#define KNI_MAX_KTHREAD 32
/*
 * Structure of port parameters
 */
struct kni_port_params {
        uint16_t port_id;    /* Port ID */
        unsigned lcore_rx;   /* lcore ID for RX */
        unsigned lcore_tx;   /* lcore ID for TX */
        uint32_t nb_lcore_k; /* Number of lcores for KNI multi kernel threads */
        uint32_t nb_kni;     /* Number of KNI devices to be created */
        unsigned lcore_k[KNI_MAX_KTHREAD];    /* lcore ID list for kthreads */
        struct rte_kni* kni[KNI_MAX_KTHREAD]; /* KNI context pointers */
} __rte_cache_aligned;

/**
 * @brief Initialize KNI subsystem
 */
void init_kni(uint16_t num_of_kni_ports);

#endif /* !KNI_H */
