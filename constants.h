#ifndef __constants_h_included__
#define __constants_h_included__

enum {
  MEMTYPE_RESERVED   = 0,
  MEMTYPE_FPM,
  MEMTYPE_EDO,
  MEMTYPE_PNR,
  MEMTYPE_SDR,
  MEMTYPE_ROM,
  MEMTYPE_DDRSGR,
  MEMTYPE_DDRSDR,
  MEMTYPE_DDR2SDR,
  MEMTYPE_DDR2_FB,
  MEMTYPE_DDR2_FB_PROBE,
  MEMTYPE_DDR3SDR
};

enum {
  DDR3MODULETYPE_UNDEFINED     = 0x00,
  DDR3MODULETYPE_RDIMM         = 0x01,
  DDR3MODULETYPE_UDIMM         = 0x02,
  DDR3MODULETYPE_SO_DIMM       = 0x03,
  DDR3MODULETYPE_MICRO_DIMM    = 0x04,
  DDR3MODULETYPE_MINI_RDIMM    = 0x05,
  DDR3MODULETYPE_MINI_UDIMM    = 0x06,
  DDR3MODULETYPE_MINI_CDIMM    = 0x07,
  DDR3MODULETYPE_72B_SO_UDIMM  = 0x08,
  DDR3MODULETYPE_72B_SO_RDIMM  = 0x09,
  DDR3MODULETYPE_72B_SO_CDIMM  = 0x0a,
  DDR3MODULETYPE_LRDIMM        = 0x0b,
  DDR3MODULETYPE_16B_SO_DIMM   = 0x0c,
  DDR3MODULETYPE_32B_SO_DIMM   = 0x0d
};

enum {
  CONFIG_DATA_PARITY = 1,
  CONFIG_DATA_ECC    = 2,
  CONFIG_ADDR_PARITY = 4
};

enum {
  ATTR_DDR_BUFFERED    = 1,
  ATTR_DDR_REGISTERED  = 2,

  ATTR_DDR2_REGISTERED = 1
};

#define MAX_RANKS       8

#endif
