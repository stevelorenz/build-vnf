/*
 * kni.c
 */

#include <rte_kni.h>

#include "kni.h"

void init_kni(uint16_t num_of_kni_ports) { rte_kni_init(num_of_kni_ports); }
