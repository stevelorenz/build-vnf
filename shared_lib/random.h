/*
 * About: Util functions for generate random data
 */

#ifndef RANDOM_H
#define RANDOM_H

#ifndef __TOOLS_LINUX_TYPES_H_
#include <linux/types.h>
#endif

/**
 * @brief Generate random bytes based on the current time.
 */
static void gen_rand_bytes(uint8_t* buf, uint16_t len);

/* TODO */
static void gen_rand_bytes(uint8_t* buf, uint16_t len)
{
        uint64_t ts_ns;
        ts_ns = bpf_ktime_get_ns();
}

#endif /* !RANDOM_H */
