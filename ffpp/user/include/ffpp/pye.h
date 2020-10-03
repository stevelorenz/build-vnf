/*
 * pye.h
 */

#ifndef PYE_H
#define PYE_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <ffpp/private.h>

EXPORT
int ffpp_pye_init(const char *program_name, const char *module_path);

EXPORT
int ffpp_pye_set_apu_handler(const char *module_name, const char *func_name);

EXPORT
int ffpp_pye_process_apu();

EXPORT void ffpp_pye_cleanup(void);

#endif /* !PYE_H */
