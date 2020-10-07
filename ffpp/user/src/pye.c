/*
 * pye.c
 */

#include <stdio.h>

#include <Python.h>

#include <rte_cycles.h>

#include <ffpp/private.h>
#include <ffpp/pye.h>

struct pye_ctx {
	wchar_t *program;
	PyObject *p_module;
	PyObject *apu_handler;
	PyObject *bytes_buffer;
};

// The owner of the context with the lifetime from init to cleanup.
// ALL other variables should be managed with Py_DECREF to avoid leaks.
static struct pye_ctx g_ctx = { .program = NULL,
				.p_module = NULL,
				.apu_handler = NULL };

int ffpp_pye_init(const char *program_name, const char *module_path)
{
	g_ctx.program = Py_DecodeLocale(program_name, NULL);
	if (g_ctx.program == NULL) {
		fprintf(stderr, "Can not decode the program name: %s\n",
			program_name);
		return -1;
	}
	Py_SetProgramName(g_ctx.program); // optional
	Py_Initialize();

	PyObject *sys = PyImport_ImportModule("sys");
	PyObject *path = PyObject_GetAttrString(sys, "path");
	PyList_Append(path, PyUnicode_FromString(module_path));
	Py_DECREF(sys);
	Py_DECREF(path);
	return 0;
}

int ffpp_pye_set_apu_handler(const char *module_name, const char *func_name)
{
	PyObject *pName;
	pName = PyUnicode_FromString(module_name);
	if (pName == NULL) {
		return -1;
	}
	g_ctx.p_module = PyImport_Import(pName);
	Py_DECREF(pName);
	if (g_ctx.p_module == NULL) {
		fprintf(stderr, "Failed to import the module: %s\n",
			module_name);
		return -1;
	}
	g_ctx.apu_handler = PyObject_GetAttrString(g_ctx.p_module, func_name);
	if (g_ctx.apu_handler == NULL || !PyCallable_Check(g_ctx.apu_handler)) {
		fprintf(stderr, "Failed to get the function: %s\n", func_name);
		return -1;
	}

	return 0;
}

// ISSUE: Need to handle buffer size checking.
int ffpp_pye_process_apu()
{
	PyObject *pArgs, *pValue;
	PyObject *pBytesArray;
	pBytesArray = PyByteArray_FromStringAndSize("test_string",
						    strlen("test_string"));
	pArgs = Py_BuildValue("(O)", pBytesArray);
	pValue = PyObject_CallObject(g_ctx.apu_handler, pArgs);
	Py_DECREF(pArgs);
	if (pValue != NULL && PyByteArray_Check(pValue)) {
		char *out_bytes = PyByteArray_AsString(pValue);
		printf("The output bytes are: %s\n", out_bytes);
		printf("The size of the output bytes: %lu", strlen(out_bytes));
		// MARK: The out_bytes is also freed.
		Py_DECREF(pValue);
	} else {
		return -1;
	}
	return 0;
}

void ffpp_pye_cleanup(void)
{
	rte_delay_us_sleep(1e3);
	Py_XDECREF(g_ctx.p_module);
	Py_XDECREF(g_ctx.apu_handler);
	PyGC_Collect();
	if (Py_FinalizeEx() < 0) {
		fprintf(stderr, "Failed to cleaup Python interpreter.\n");
	}
	PyMem_RawFree(g_ctx.program);
}
