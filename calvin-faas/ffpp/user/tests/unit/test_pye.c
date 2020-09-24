/*
 * test_pye.c
 *
 * Test embedded CPython interpretor.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/**
 * Use to execute a Python script without need to interact with the main
 * application.
*/
int test_python_embedding(int argc, char *argv[])
{
	wchar_t *program = Py_DecodeLocale(argv[0], NULL);
	if (program == NULL) {
		fprintf(stderr, "Can not decode argv[0], argc: %d\n", argc);
		exit(1);
	}
	Py_SetProgramName(program); // optional
	Py_Initialize();
	PyRun_SimpleString("from time import time,ctime\n"
			   "import numpy as np\n"
			   "print('Today is: %s' % ctime(time()))\n"
			   "print('%.2f' % np.average([1, 2, 3]))\n");

	const char *file_path = "/ffpp/user/tests/data/test_pye.py";
	FILE *f = fopen(file_path, "r");
	PyRun_SimpleFile(f, file_path);

	fclose(f);

	PyObject *sys = PyImport_ImportModule("sys");
	PyObject *path = PyObject_GetAttrString(sys, "path");
	PyList_Append(path, PyUnicode_FromString("/ffpp/user/tests/data"));
	Py_DECREF(sys);
	Py_DECREF(path);

	PyObject *pName, *pModule;
	pName = PyUnicode_FromString("test_pye");
	if (pName == NULL) {
		Py_Finalize();
	}
	pModule = PyImport_Import(pName);
	Py_DECREF(pName);

	if (pModule != NULL) {
		printf("The module is loaded successfully.\n");
		PyObject *pFunc;
		pFunc = PyObject_GetAttrString(pModule, "handle_bytesarray");
		if (pFunc && PyCallable_Check(pFunc)) {
			PyObject *pArgs, *pValue;
			PyObject *testList;
			PyObject *pBytesArray;
			pBytesArray = PyByteArray_FromStringAndSize(
				"test_string", strlen("test_string"));
			pArgs = Py_BuildValue("(O)", pBytesArray);
			pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pArgs);
			if (pValue != NULL && PyByteArray_Check(pValue)) {
				char *out_bytes = PyByteArray_AsString(pValue);
				printf("The output bytes are: %s\n", out_bytes);
				printf("The size of the output bytes: %lu",
				       strlen(out_bytes));
				// MARK: The owner of out_bytes is pValue.
				Py_DECREF(pValue);
			} else {
				Py_XDECREF(pFunc);
				Py_XDECREF(pModule);
				if (PyErr_Occurred()) {
					PyErr_Print();
				}
				fprintf(stderr,
					"Failed to call the funcion.\n");
				return 1;
			}
		} else {
			if (PyErr_Occurred()) {
				PyErr_Print();
			}
			fprintf(stderr, "Can not find the function.\n");
		}
		Py_XDECREF(pFunc);
		Py_XDECREF(pModule);
	} else {
		fprintf(stderr, "Failed to load the module.\n");
		return 1;
	}

	if (Py_FinalizeEx() < 0) {
		exit(120);
	}
	PyMem_RawFree(program);
	return 0;
}

int main(int argc, char *argv[])
{
	// When running test with `meson test`, the current dir is
	// /ffpp/user/build inside the dev container.
	test_python_embedding(argc, argv);
	return 0;
}
