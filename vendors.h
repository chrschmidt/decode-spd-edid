#ifndef __vendors_h_included__
#define __vendors_h_included__

#define JEDEC_BANKS 11

const char *get_vendor (const unsigned char vendor_id[JEDEC_BANKS]);
const char *get_vendor64 (const unsigned char vendor_id[8]);
const char *get_vendor16 (const unsigned int vendor_id);

#endif
