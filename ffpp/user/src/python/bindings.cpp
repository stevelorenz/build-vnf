// TODO (Maybe): Add Python bindings to FFPP.

#include <pybind11/pybind11.h>

namespace py = pybind11;

int add(int i, int j)
{
	return i + j;
}

PYBIND11_MODULE(pyffpp, m)
{
	m.doc() = "pybind11 FFPP plugin";

	m.def("add", &add, "A function which adds two numbers");
}