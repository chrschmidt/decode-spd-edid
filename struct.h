#ifndef __struct_h_included__
#define __struct_h_included__

#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#pragma pack(1)
/*
 * JEDEC Standard No. 21-C, Page 4.1.2.5-1 (Annex E)
 * Specific PDs for Synchronous DRAM (SDRAM)
 * 4_01_02_05R11.pdf
 *
 * JEDEC Standard No. 21-C, Page 4.1.2.4-1 (Annex D)
 * SPDs for DDR SDRAM
 * 4_01_02_04R13.pdf
 *
 * JEDEC Standard No. 21-C, Page 4.1.2.10-1 (Annex X)
 * Specific SPDs for DDR2 SDRAM
 * 4_01_02_10R17.pdf
 */
struct sdram_spd {
    /* 0x00 */
    u8 bytes_written;
    u8 total_bytes;
    u8 memory_type;
    u8 num_row_addr;
    u8 num_col_addr;
    u8 num_ranks;
    u8 data_width;              /* u16 for SDR / DDR */
    u8 reserved1;
    /* 0x08 */
    u8 voltage_level;
    u8 min_clk_cycle_cl_max_0;
    u8 access_from_clock;
    u8 config_type;
    u8 refresh_type;
    u8 primary_width;
    u8 error_checking_width;
    u8 min_clk_delay;           /* reserved for ddr2 */
    /* 0x10 */
    u8 burst_length;
    u8 num_banks_device;
    u8 cas_latency;
    u8 cs_latency;              /* reserved for ddr2 */
    union {
        u8 we_latency;          /* ddr */
        u8 dimm_type;           /* ddr2 */
    };
    u8 module_attr;
    u8 module_attr_general;
    u8 min_clk_cycle_cl_max_1;  /* 0.5 clk less than max for DDR, 1 clk for SDR */
    /* 0x18 */
    u8 max_tac_cl_05;
    u8 min_clk_cycle_cl_max_2;  /* 1 clk less than max for DDR, 2 clk for SDR */
    u8 max_tac_cl_1;
    u8 min_trp;
    u8 min_trrd;
    u8 min_trcd;
    u8 min_tras;
    u8 bank_density;
    /* 0x20 */
    u8 ctrl_setup_time;
    u8 ctrl_hold_time;
    u8 data_setup_time;
    u8 data_hold_time;
    u8 reserved2[4];
    u8 trc_trfc_ext;            /* only from DDR2 upward */
    /* 0x29 */
    u8 min_trc;
    u8 min_trfc;                /* only from DDR upward */
    u8 max_tck;                 /* only from DDR upward */
    u8 max_tdqsq;               /* only from DDR upward */
    u8 max_tqhs;                /* only from DDR upward */
    u8 reserved3;
    u8 attr_dimm_height;        /* only from DDR upward */
    /* 0x30 */
    u8 reserved4[14];
    /* 0x3e */
    u8 spd_revision;
    u8 checksum;
    /* 0x40 */
    u8 manufacturer_jedec_id[8];
    /* 0x48 */
    u8 manufacturing_location;
    u8 part_number[18];
    u16 module_revision;
    u16 manufacturing_date;
    u32 serial;
    u8 manufacturer_specific[29];       /* 27 manufacturer / 2 vendor for SDR */
    /* 0x80 */
    u8 customer_specific[128];
};

/*
 * Intel Extreme Memory Profile (Intel XMP) Specification Revision 1.1
 */
struct ddr3_xmp_profile {
    u8 voltage;
    u8 min_tck;
    u8 min_taa;
    u16 cas_latency;
    u8 min_tcwl;
    u8 min_trp;
    u8 min_trcd;
    u8 min_twr;
    u8 min_tras_trc_upper_nibble;
    u8 min_tras_lsb;
    u8 min_trc_lsb;
    u16 max_trefi;
    u16 min_trfc;
    u8 min_trtp;
    u8 min_trrd;
    u8 min_tfaw_upper_nibble;
    u8 min_tfaw_lsb;
    u8 min_twtr;
    u8 turnaround;
    u8 tccd_optimize;
    u8 cmd_rate;
    u8 self_refresh;
    u8 reserved[9];
    u8 vendor_personality;
};

struct ddr3_xmp {
    u16 id;
    u8 profile_org_conf;
    u8 revision;
    u8 p1_mtb_dividend;
    u8 p1_mtb_divisor;
    u8 p2_mtb_dividend;
    u8 p2_mtb_divisor;
    u8 reserved;
    struct ddr3_xmp_profile profiles[2];
    u8 unused;
};

/*
 * JEDEC Standard No. 21-C, Page 4.1.2.11-1 (Annex K)
 * Serial Presence Detect (SPD) for DDR3 SDRAM Modules
 * 4_01_02_11R18.pdf
 */
struct ddr3_sdram_spd {
    /* 0x00 */
    u8 bytes_used_crc;
    u8 spd_revision;
    u8 memory_type;
    u8 module_type;
    u8 density_banks;
    u8 adressing;
    u8 voltage;
    u8 organization;
    /* 0x08 */
    u8 bus_width;
    u8 ftb_dividend_divisor;
    u8 mtb_dividend;
    u8 mtb_divisor;
    u8 min_tck;
    u8 reserved1;
    u16 cas_latency;
    /* 0x10 */
    u8 min_taa;
    u8 min_twr;
    u8 min_trcd;
    u8 min_trrd;
    u8 min_trp;
    u8 min_tras_trc_upper_nibble;
    u8 min_tras_lsb;
    u8 min_trc_lsb;
    /* 0x18 */
    u16 min_trfc;
    u8 min_twtr;
    u8 min_trtp;
    u8 min_tfaw_upper_nibble;
    u8 min_tfaw_lsb;
    u8 optional_features;
    u8 thermal_refresh;
    /* 0x20 */
    u8 thermal_sensor;
    u8 device_type;
    u8 reserved2[26];
    u8 module_specific[57];
    u16 manufacturer_jedec_id;
    u8 manufacturing_location;
    /* 0x78 */
    u16 manufacturing_date;
    u32 serial_number;
    u16 crc;
    /* 0x80 */
    u8 part_number[18];
    u16 module_revison;
    u16 dram_manufacturer_jedec_id;
    u8 manufacturer_specific[26];
    union {
        u8 customer_specific[80];
        struct ddr3_xmp xmp;
    };
};

/*
 * JEDEC Standard No. 21-C, Page 4.1.2.12-1 (Annex L)
 * Serial Presence Detect (SPD) for DDR4 SDRAM Modules
 * 4_01_02_AnnexL-4R27.pdf
 */
struct ddr4_sdram_ext_unbuffered {
    u8 raw_card_extension_height;
    u8 thickness;
    u8 reference_raw_card;
    u8 address_mapping;
    u8 reserved[122];
    u16 crc;
};

struct ddr4_sdram_spd {
    /* 0x00 */
    u8 bytes_used_crc;
    u8 spd_revision;
    u8 memory_type;
    u8 module_type;
    u8 density_banks;
    u8 adressing;
    u8 primary_package;
    u8 optional1;
    /* 0x08 */
    u8 thermal_refresh;
    u8 optional2;
    u8 secondary_package;
    u8 voltage;
    u8 organization;
    u8 bus_width;
    u8 thermal_sensor;
    u8 extended_module_type;
    /* 0x10 */
    u8 reserved2;
    u8 timebases;
    u8 min_tckavg;
    u8 max_tckavg;
    u8 cas_latencies[4];
    /* 0x18 */
    u8 min_taa;
    u8 min_trcd;
    u8 min_trp;
    u8 min_tras_trc_upper;
    u8 min_tras_lower;
    u8 min_trc_lower;
    u16 min_trfc1;
    /* 0x20 */
    u16 min_trfc2;
    u16 min_trfc4;
    u16 min_tfaw;
    u8 min_trrd_s;
    u8 min_trrd_l;
    /* 0x28 */
    u8 min_tcdd_l;
    u8 reserved3[19];
    /* 0x3c */
    u8 bit_mapping[18];
    /* 0x4e */
    u8 reserved4[39];
    /* 0x75 */
    u8 fine_min_tccd_l;
    u8 fine_min_trrd_l;
    u8 fine_min_trrd_s;
    /* 0x78 */
    s8 fine_min_trc;
    s8 fine_min_trp;
    s8 fine_min_trcd;
    s8 fine_min_taa;
    s8 fine_max_tckavg;
    s8 fine_min_tckavg;
    u16 crc;
    /* 0x80 */
    union {
        u8 module_specific[128];
        struct ddr4_sdram_ext_unbuffered ext_ub;
    };
    /* 0x100 */
    u8 reserved5[64];
    /* 0x140 */
    u16 manufacturer_jedec_id;
    u8 manufacturing_location;
    u16 manufacturing_date;
    u32 serial_number;
    u8 part_number[20];
    u8 module_revison;
    u16 dram_manufacturer_jedec_id;
    u8 dram_stepping;
    u8 manufacturer_specific[29];
    u8 reserved6[2];
    /* 0x180 */
    u8 end_user_data[128];
};
#pragma pack()

#endif
