#ifndef __ddr4_h_included__
#define __ddr4_h_included__

#include "struct.h"

int get_ddr4_memreq (const struct ddr4_sdram_spd * eeprom, int length);
void do_ddr4 (const struct ddr4_sdram_spd * eeprom, int length);

#endif
