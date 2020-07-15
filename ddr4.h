#pragma once

#include "struct.h"

int get_ddr4_memreq (const struct ddr4_sdram_spd * eeprom, int length);
void do_ddr4 (const struct ddr4_sdram_spd * eeprom, int length);
