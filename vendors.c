#include <string.h>
#include "vendors.h"

#include "vendortable.h"

static const int num_vendors = sizeof (jedec_vendors) / sizeof (jedec_vendors[0]);

const char *get_vendor64 (const unsigned char vendor_id[8]) {
    int i;

    for (i=0; i<8; i++)
        if (vendor_id[i] != 0x7f)
            break;

    return get_vendor16 (256*i+vendor_id[i]);
}

const char *get_vendor16 (const unsigned int vendor_id) {
    int i;

    for (i=0; i<num_vendors; i++)
        if (jedec_vendors[i].id == vendor_id)
            return jedec_vendors[i].name;

    return "Unknown";
}
