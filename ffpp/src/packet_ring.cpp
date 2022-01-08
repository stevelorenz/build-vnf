/*
 * packet_ring.cpp
 */

#include "ffpp/packet_ring.hpp"

namespace ffpp
{

PacketRing::PacketRing(std::string name, uint64_t count, uint64_t socket_id)
{
	ring_ = rte_ring_create(name.c_str(), count, socket_id,
				RING_F_SP_ENQ | RING_F_SC_DEQ);
}

PacketRing::~PacketRing()
{
	rte_ring_free(ring_);
}

bool PacketRing::empty() const
{
	auto ret = rte_ring_empty(ring_);
	if (ret == 1) {
		return true;
	} else {
		return false;
	}
}

bool PacketRing::full() const
{
	auto ret = rte_ring_full(ring_);
	if (ret == 1) {
		return true;
	} else {
		return false;
	}
}

uint64_t PacketRing::count() const
{
	return rte_ring_count(ring_);
}

uint64_t PacketRing::capacity() const
{
	return rte_ring_get_capacity(ring_);
}

struct rte_mbuf *PacketRing::pop()
{
	void *p = nullptr;
	auto ret = rte_ring_dequeue(ring_, &p);
	if (ret == 0) {
		return static_cast<struct rte_mbuf *>(p);
	}
	return nullptr;
}

bool PacketRing::push(struct rte_mbuf *m)
{
	auto ret = rte_ring_enqueue(ring_, &m);
	if (ret == 0) {
		return true;
	} else {
		return false;
	}
}

} // namespace ffpp
