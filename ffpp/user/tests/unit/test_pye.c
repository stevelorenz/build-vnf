/*
 * test_pye.c
 *
 * Test embedded CPython interpretor.
 */

#include <ffpp/pye.h>

/**
 * Use to execute a Python script without need to interact with the main
 * application.
*/
int test_python_embedding(int argc, char *argv[])
{
	(void)(argc);
	ffpp_pye_init(argv[0], "/ffpp/user/tests/data");
	ffpp_pye_set_apu_handler("test_pye", "handle_bytesarray");
	ffpp_pye_process_apu();

	ffpp_pye_cleanup();
	return 0;
}

// MARK: Py_FinalizeEx() currently leak a fixed amount of memory.
/* const char *__asan_default_options() */
/* { */
/* 	return "detect_leaks=0"; */
/* } */

int main(int argc, char *argv[])
{
	// When running test with `meson test`, the current dir is
	// /ffpp/user/build inside the dev container.
	test_python_embedding(argc, argv);
	return 0;
}
