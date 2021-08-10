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
#include <stdio.h>

#include <iostream>
#include <numeric>
#include <vector>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

static const char *test_py_path = "/ffpp/tests/data";

void print_in_embeded()
{
	std::cout << "Print inside the embeded module." << std::endl;
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
