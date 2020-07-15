#pragma once

#include "eedid_struct.h"

int get_eedid_memreq (const struct eedid_t * eeprom, int length);
void do_eedid (const struct eedid_t * eeprom, int length);
