/*
 * device.h
 */

#ifndef DEVICE_H
#define DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * Device API
 *
 */

#define FFPP_MAX_PORTS 8

/**
 * struct ffpp_dpdk_device_config - DPDK device configuration
 */
struct ffpp_dpdk_device_config {
	uint32_t port_id;
	struct rte_mempool **pool;
	uint16_t rx_queues;
	uint16_t tx_queues;
	uint16_t rx_descs;
	uint16_t tx_descs;
	uint8_t drop_enabled;
	uint8_t disable_offloads;
};

/**
 * ffpp_dpdk_init_device() - Initialize a Ethernet device.
 *
 * @param cfg: Device configuration packed in a ffpp_dpdk_device_config struct
 *
 * @return: Flag
 */
int ffpp_dpdk_init_device(struct ffpp_dpdk_device_config *cfg);

/**
 * ffpp_dpdk_cleanup_devices - Cleanup all initialized Ethernet devices.
 */
void ffpp_dpdk_cleanup_devices(void);

struct rx_queue {
	uint16_t id;
};

struct tx_queue {
	uint16_t id;
	uint16_t len;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !DEVICE_H */
