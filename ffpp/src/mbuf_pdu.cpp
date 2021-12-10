/**
 *  Copyright (C) 2020 Zuo Xiang
 *  All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#include <cassert>

#include <gsl/gsl>
#include <fmt/core.h>
#include <fmt/format.h>
#include <glog/logging.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <tins/tins.h>

#include "ffpp/mbuf_pdu.hpp"

namespace ffpp
{
int write_eth_to_mbuf(Tins::EthernetII &eth, struct rte_mbuf *mbuf)
{
	// Remove current payload in the given mbuf
	int ret;
	ret = rte_pktmbuf_trim(mbuf, rte_pktmbuf_pkt_len(mbuf));
	assert(ret == 0);

	char *p = nullptr;
	// The checksums should be already calculated here
	// MARK: This serialize() is the bottleneck according to the
	// micro benchmark.
	// eth_to_mbuf now takes twice as long as the read_mbuf_to_eth...
	auto payload = eth.serialize();
	p = rte_pktmbuf_append(mbuf, payload.size());
	if (unlikely(p == nullptr)) {
		LOG(ERROR)
			<< "There is not enough tailroom space in the given mbuf!";
		return -1;
	}
	rte_memcpy(p, payload.data(), payload.size());
	return 0;
}

Tins::EthernetII read_mbuf_to_eth(const struct rte_mbuf *m)
{
	uint8_t *p = nullptr;
	p = rte_pktmbuf_mtod(m, uint8_t *);

	Tins::EthernetII eth = Tins::EthernetII(p, rte_pktmbuf_pkt_len(m));

	return eth;
}
} // namespace ffpp
