/*
 * device.h
 */

#ifndef DEVICE_H
#define DEVICE_H

/**
 * @brief
 */
struct dpdk_device_config {
        uint32_t portid;
        struct rte_mempool** pool;
        uint16_t rx_queues;
        uint16_t tx_queues;
        uint16_t rx_descs;
        uint16_t tx_descs;
        uint8_t drop_enabled;
        uint8_t disable_offloads;
};

/**
 * @brief dpdk_init_device
 *
 * @param cfg
 *
 * @return
 */
int dpdk_init_device(struct dpdk_device_config* cfg);

#endif /* !DEVICE_H */
