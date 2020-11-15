/*
 * test_pye_pybind11.cpp
 *
 * Test embedding Python interpreter.
 */

#include <cassert>
#include <stdio.h>

#include <iostream>
#include <numeric>
#include <vector>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

static const char *test_py_path = "/ffpp/user/tests/data";

void print_in_embeded()
{
	printf("Print inside the embeded module.\n");
}

// Add an embedded module
PYBIND11_EMBEDDED_MODULE(embeddedmodule, module)
{
	module.doc() = "Embeded Module.";
	module.def("print_in_embeded", &print_in_embeded);
}

int main()
{
	py::scoped_interpreter guard{};
	py::exec("print('Hello Pybind11 !')");
	py::exec("import embeddedmodule\nembeddedmodule.print_in_embeded()");

	// Load python script from the disk
	auto sys = py::module::import("sys");
	sys.attr("path").attr("insert")(0, test_py_path);
	auto test_pye_module = py::module::import("test_pye");
	auto multiply_func = test_pye_module.attr("multiply");
	auto ret_object = multiply_func(2, 3);
	int ret = ret_object.cast<int>();
	assert(ret == 6);

	// Exchange STL containers with Python.
	std::vector<int> numbers(10);
	std::iota(std::begin(numbers), std::end(numbers), 0);
	auto print_vector_func = test_pye_module.attr("print_vector");
	ret_object = print_vector_func(numbers);
	std::vector<std::vector<int> > nested;
	nested.push_back(numbers);
	nested.push_back(numbers);
	ret_object = print_vector_func(nested);
}
