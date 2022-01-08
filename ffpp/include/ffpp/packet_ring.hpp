/**
 *  Copyright (C) 2021 Zuo Xiang
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

#pragma once

#include <cstdint>

#include <string>

#include <rte_mbuf.h>
#include <rte_ring.h>

/**
 * @file
 * PacketRing
 *
 */

namespace ffpp
{

/**
 * A wrapper for rte_ring
 */
class PacketRing {
    public:
	PacketRing(std::string name, uint64_t count, uint64_t socket_id);
	virtual ~PacketRing();

	bool empty() const;

	bool full() const;

	uint64_t count() const;

	uint64_t size() const
	{
		return count();
	};

	uint64_t capacity() const;

	struct rte_mbuf *pop();

	bool push(struct rte_mbuf *m);

    private:
	struct rte_ring *ring_;
};

} // namespace ffpp
