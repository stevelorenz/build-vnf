/*
 * test_packet_container.cpp
 */

#include <cstdint>
#include <iostream>

#include <gtest/gtest.h>

#include "ffpp/packet_engine.hpp"
#include "ffpp/packet_ring.hpp"

static auto gPE = ffpp::PacketEngine("/ffpp/tests/unit/test_config.yaml");

TEST(UnitTest, TestPacketRing)
{
	using namespace ffpp;
	// The EAL must be initialized before creating a PacketRing
	PacketRing ring = PacketRing("test_ring", 64, 0);
	ASSERT_TRUE(ring.empty());
	ASSERT_FALSE(ring.full());
	ASSERT_EQ(ring.count(), (uint64_t)(0));
	ASSERT_EQ(ring.size(), (uint64_t)(0));
	ASSERT_EQ(ring.capacity(), (uint64_t)(63));
}
