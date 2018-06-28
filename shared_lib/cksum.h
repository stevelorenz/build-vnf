/*
 * cksum.h
 * About: Helper functions for IP/TCP/UDP header checksums
 */

#ifndef CKSUM_H
#define CKSUM_H

#ifndef __TOOLS_LINUX_TYPES_H_
#include <linux/types.h>
#endif

#ifndef _LINUX_IN_H
#include <linux/in.h>
#endif

/**
 * @brief Calculate the checksum for IP/TCP/UDP headers
 *        type: 0->IP, 1->UDP, 2->TCP
 *
 * @return The checksum with unsigned integer format.
 */
static inline uint16_t cksum(uint8_t* buf, uint16_t len, uint8_t type);

static inline uint16_t cksum(uint8_t* buf, uint16_t len, uint8_t type)
{
        uint32_t sum = 0;

        if (type == 1) {
                sum += IPPROTO_UDP;
                sum += len - 8;
        }
        if (type == 2) {
                sum += IPPROTO_TCP;
                sum += len - 8;
        }
        // build the sum of 16bit words
        while (len > 1) {
                sum += 0xffff & (*buf << 8 | *(buf + 1));
                buf += 2;
                len -= 2;
        }
        // if there is a byte left then add it (padded with zero)
        if (len) {
                sum += 0xffff & (*buf << 8 | 0x00);
        }
        // // now calculate the sum over the bytes in the sum
        // // until the result is only 16bit long
        sum = (sum & 0xffff) + (sum >> 16);
        sum = (sum & 0xffff) + (sum >> 16);

        // // build 1's complement:
        return ((uint16_t)sum ^ 0xffff);
}

#endif /* !CKSUM_H */
