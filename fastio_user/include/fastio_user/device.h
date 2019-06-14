/*
 * device.h
 */

#ifndef DEVICE_H
#define DEVICE_H

/**
 * @file
 *
 * Device API
 *
 */

/**
 * struct dpdk_device_config - DPDK device configuration
 */
struct dpdk_device_config {
        uint32_t port_id;
        struct rte_mempool** pool;
        uint16_t rx_queues;
        uint16_t tx_queues;
        uint16_t rx_descs;
        uint16_t tx_descs;
        uint8_t drop_enabled;
        uint8_t disable_offloads;
};

/**
 * dpdk_init_device() - Initialize a Ethernet device.
 *
 * @param cfg: Device configuration packed in a dpdk_device_config struct
 *
 * @return: Flag
 */
int dpdk_init_device(struct dpdk_device_config* cfg);

/**
 * dpdk_cleanup_devices - Cleanup all initialized Ethernet devices.
 */
void dpdk_cleanup_devices(void);

#endif /* !DEVICE_H */
