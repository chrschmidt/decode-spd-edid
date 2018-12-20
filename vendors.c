#include <string.h>
#include "vendors.h"

struct jedec_vendor {
    const unsigned char id[JEDEC_BANKS];
    const char *name;
};

static const struct jedec_vendor jedec_vendors[] = {
#include "vendortable.h"
    {{0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F}, 0}
};

const char *get_vendor (const unsigned char vendor_id[JEDEC_BANKS]) {
    int i, j;

    for (i = 0; jedec_vendors[i].name; i++) {
        for (j = 0; j < JEDEC_BANKS; j++) {
            if (vendor_id[j] != jedec_vendors[i].id[j])
                break;
            if (vendor_id[j] != 0x7F)
                return jedec_vendors[i].name;
        }
    }
    return "unknown";
}

const char *get_vendor64 (const unsigned char vendor_id[8]) {
    unsigned char jedec_vendor_id[JEDEC_BANKS];

    memset (jedec_vendor_id, 0x7F, JEDEC_BANKS);
    memcpy (jedec_vendor_id, vendor_id, 8);
    return get_vendor (jedec_vendor_id);
}

const char *get_vendor16 (const unsigned int vendor_id) {
    unsigned char vendor_id_long[JEDEC_BANKS];
    int i;

    bzero (vendor_id_long, sizeof (vendor_id_long));
    if ((vendor_id & 127) > JEDEC_BANKS)
        return "invalid";
    for (i = 0; i < (vendor_id & 127); i++)
        vendor_id_long[i] = 0x7F;
    vendor_id_long[vendor_id & 127] = vendor_id >> 8;

    return get_vendor (vendor_id_long);
}
