/*
 * test_data_processor.cpp
 */

#include <cstdint>

#include <gtest/gtest.h>
#include <pybind11/embed.h>

#include <ffpp/packet_engine.hpp>
#include <ffpp/data_processor.hpp>

namespace py = pybind11;

static auto gPE = ffpp::PacketEngine("/ffpp/tests/unit/test_config.yaml");

TEST(UnitTest, TestPyDataProcessor)
{
	using namespace ffpp;
	py_insert_sys_path("/ffpp/tests/unit/", 0);

	auto test_module = py::module::import("test_py_data_processor");
	auto plus_one = test_module.attr("plus_one");
	auto ret = plus_one(1);
	uint64_t n = ret.cast<uint64_t>();
	ASSERT_EQ(n, uint64_t(2));

	auto append_test_str = test_module.attr("append_test_str");
	ret = append_test_str("zuozuo");
	std::string new_str = ret.cast<std::string>();
	ASSERT_EQ(new_str, "zuozuo_fanfan");
}
