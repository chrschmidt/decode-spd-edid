#include <math.h>
#include <string.h>
#include <stdio.h>
#include "constants.h"
#include "struct.h"
#include "output.h"
#include "vendors.h"
#include "ddr4.h"

static const char *moduletypenames[] = {
    "Extended", "RDIMM", "UDIMM", "SO-DIMM",
    "LRDIMM", "Mini-RDIMM", "Mini-UDIMM", "Reserved",
    "72b-SO-RDIMM", "72b-SO_UDIMM", "Reserved", "Reserved",
    "16b-SO-DIMM", "32b-SO_DIMM", "Reserved", "Reserved"
};

int get_ddr4_density (char density) {
    switch (density & 15) {
    case  0: return 256;
    case  1: return 512;
    case  2: return 1024;
    case  3: return 2048;
    case  4: return 4096;
    case  5: return 8192;
    case  6: return 16384;
    case  7: return 32768;
    case  8: return 12288;
    case  9: return 24576;
    }
    return -1;
 }

int get_ddr4_secondary_density (char density, char ratio) {
    switch ((ratio >> 2) & 3) {
    case 0: return get_ddr4_density (density);
    case 1: switch (density & 15) {
        case  1: return 256;
        case  2: return 512;
        case  3: return 1024;
        case  4: return 2048;
        case  5: return 4096;
        case  6: return 12288;
        case  7: return 24576;
        case  8: return 8192;
        case  9: return 16384;
        }
        break;
    case 2: switch (density & 15) {
        case  2: return 256;
        case  3: return 512;
        case  4: return 1024;
        case  5: return 2048;
        case  6: return 8192;
        case  7: return 16384;
        case  8: return 4096;
        case  9: return 12288;
        }
        break;
    }
    return -1;
}

const char * get_ddr4_package (char package) {
    unsigned char loading = package & 3;
    unsigned char diecount = (package >> 4) & 7;
    static char retval[40];
    switch (loading) {
    case 0: if (package & 128) return "Unspecified Non-Monolithic Device";
            else return "SDP (Single Die Package)";
    case 1: switch (diecount) {
        case 1: return "DDP (Dual Die Package)";
        case 2: return "QDP (Quad Die Package)";
        default: return "Unknown Multi Die Package";
        }
    case 2: switch (diecount) {
        case 0: return "Unknown single load stack";
        default:
            sprintf (retval,
                     "%dH 3DS (%d SDRAM die single load stack)",
                     diecount, diecount);
            return retval;
        }
    }
    return "(Reserved)";
}

static int ddr4_bytesused (const unsigned char x) {
    switch (x & 15) {
    case 1:
        return 128;
    case 2:
        return 256;
    case 3:
        return 384;
    case 4:
        return 512;
    default:
        return -1;
    }
}

static int ddr4_bytestotal (const unsigned char x) {
    switch ((x >> 4) & 7) {
    case 1:
        return 256;
    case 2:
        return 512;
    default:
        return -1;
    }
}

static int ddr4_crc (const char *data, int count) {
    int crc, i;
    crc = 0;
    while (--count >= 0) {
        crc = crc ^ (int) *data++ << 8;
        for (i = 0; i < 8; i++)
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
           else
                crc = crc << 1;
    }
    return (crc & 0xFFFF);
}

int get_ddr4_memreq (const struct ddr4_sdram_spd *eeprom, int length) {
    if (length == 0)
	return -1;
    return ddr4_bytestotal (eeprom->bytes_used_crc);
}

static int ps_to_clocks (const int minimum, const int period) {
    return (((minimum * 1000) / period) + 974) / 1000;
}

static const char * get_thickness (int value) {
    static char buffer [16];
    switch (value) {
    case 0: return ("<= 1mm");
    case 15: return ("> 15mm");
    default:
        sprintf (buffer, "%dmm - %dmm",
                 (value & 15), (value & 15) + 1);
    }
    return buffer;
}

void do_ddr4 (const struct ddr4_sdram_spd *eeprom, int length) {

    int i;
    int bankgroups, banks, ranks, rows, columns;
    int width, size;
    int mtb, ftb;

    char linebuf[200], linebuf2[256];
    int bytes_used = 0;
    int checksum, checksum2;

    const int ddr4_frequencies[] = { 1600, 1466, 1333, 1200, 933, 667 };
    const int ddr4_periods[] = {  625, /* 1600MHz = 625ps */
                                  682, /* 1466MHz = 682.1ps */
                                  750, /* 1333MHz = 750.2ps */
                                  833, /* 1200MHz = 833.3ps */
                                 1072, /*  933MHz = 1071.8ps */
                                 1499  /*  667MHz = 1499.3ps */
    };
    const int num_ddr4_frequencies = sizeof (ddr4_frequencies) / sizeof (ddr4_frequencies[0]);

    if (length < 256) {
        printf ("Insufficient data read, aborting decode\n");
        return;
    }
    /* SPD information */
    sprintf (linebuf, "%d.%d", eeprom->spd_revision >> 4,
             eeprom->spd_revision & 15);
    if (eeprom->bytes_used_crc & 15 && eeprom->bytes_used_crc & 112) {
        bytes_used = ddr4_bytesused (eeprom->bytes_used_crc);
        sprintf (linebuf2, ", %d/%d bytes used",
                 bytes_used, ddr4_bytestotal (eeprom->bytes_used_crc));
        strcat (linebuf, linebuf2);
    }
    do_line ("SPD Revision:", linebuf);
    checksum = ddr4_crc ((char *) eeprom, 126);
    sprintf (linebuf, "%04X, %scorrect", checksum,
             checksum == eeprom->crc ? "" : "not ");
    do_line ("Checksum", linebuf);

    /* Vendor information */
    if (bytes_used > 256) {
        if (length >= 384) {
            sprintf (linebuf, "%s (%04x)",
                     get_vendor16 (eeprom->manufacturer_jedec_id),
                     eeprom->manufacturer_jedec_id);
            do_line ("Module Vendor", linebuf);
            if (eeprom->dram_manufacturer_jedec_id) {
                sprintf (linebuf, "%s (%04x)",
                         get_vendor16 (eeprom->dram_manufacturer_jedec_id),
                         eeprom->dram_manufacturer_jedec_id);
                do_line ("Chip Vendor", linebuf);
            }
            memcpy (linebuf, &(eeprom->part_number), 20);
            linebuf[20] = 0;
            do_line ("Part Number", linebuf);
        } else {
            printf ("Vendor information not available, insufficient data read\n");
        }
    }

    /* general module type */
    ranks = ((eeprom->organization >> 3) & 7) + 1;
    rows = ((eeprom->adressing >> 3) & 7) + 12;
    columns = (eeprom->adressing & 7) + 9;
    bankgroups = 1 << ((eeprom->density_banks >> 6) & 3);
    banks = 4 << ((eeprom->density_banks >> 4) & 3);
    width = (8 << (eeprom->bus_width & 7)) + (((eeprom->bus_width >> 3) & 3) == 1 ? 8 : 0);
    size  = ranks * (1 << (rows + columns - 20)) * bankgroups * banks * (width >> 3);

    strcpy (linebuf, "DDR4 ");
    if (eeprom->module_type & 15)
        strcat (linebuf, moduletypenames[eeprom->module_type & 15]);

    if (((eeprom->bus_width >> 3) & 3) == 1)
        snprintf (linebuf2, sizeof (linebuf2) - 1, "%s (ECC) %d/%dMB",
                  linebuf, size * 8 / 9, size);
    else
        snprintf (linebuf2, sizeof (linebuf2) - 1, "%s %dMB", linebuf, size);
    do_line ("Part Type", linebuf2);

    switch (eeprom->module_type & 15) {
    case DDR4MODULETYPE_UDIMM:
    case DDR4MODULETYPE_SO_DIMM:
    case DDR4MODULETYPE_MINI_UDIMM:
    case DDR4MODULETYPE_72B_SO_UDIMM:
    case DDR4MODULETYPE_16B_SO_DIMM:
    case DDR4MODULETYPE_32B_SO_DIMM:
        checksum2 = ddr4_crc ((char *) eeprom->module_specific, 126);
        sprintf (linebuf, "%04X, %scorrect", checksum2,
                 checksum2 == eeprom->ext_ub.crc ? "" : "not ");
        do_line ("Extension Checksum", linebuf);
        switch (eeprom->ext_ub.raw_card_extension_height & 31) {
        case 0:
            sprintf (linebuf, "<= 15mm");
            break;
        case 31:
            sprintf (linebuf, ">= 45mm");
            break;
        default:
            sprintf (linebuf, "%dmm - %dmm",
                     14 + (eeprom->ext_ub.raw_card_extension_height & 31),
                     15 + (eeprom->ext_ub.raw_card_extension_height & 31));
        }
        sprintf (linebuf2, "Height: %s, Thickness above PCB: Front %s, Back %s",
                 linebuf,
                 get_thickness (eeprom->ext_ub.thickness & 15),
                 get_thickness (eeprom->ext_ub.thickness >> 4));
        do_line ("Dimensions", linebuf2);
        break;
    }

    /* voltage */
    linebuf[0] = 0;
    if (eeprom->voltage & 1) addlist (linebuf, "1.2V operable");
    if (eeprom->voltage & 2) addlist (linebuf, "1.2V endurant");
    if (eeprom->voltage & 0xfc) addlist (linebuf, "(Unknown)");
    do_line ("Voltage", linebuf);

    /* organisation */
    snprintf (linebuf, sizeof (linebuf) - 1,
              "%d rank%s, %d bank group%s, %d bank%s%s, %d rows/%d columns (%dMBitx%d per bank group)",
              ranks, ranks == 1 ? "" : "s",
              bankgroups, bankgroups == 1 ? "" : "s",
              banks, banks == 1 ? "" : "s", bankgroups > 1 ? " each" : "",
              rows, columns, (1 << (rows + columns - 20)) * banks, width);
    do_line ("Organisation", linebuf);


    /* chip organisation */
    snprintf (linebuf, sizeof (linebuf) - 1,
              "%s, %sBitx%d%s",
              get_ddr4_package (eeprom->primary_package),
              mtostr (get_ddr4_density (eeprom->density_banks)),
              4 << (eeprom->organization & 7),
              eeprom->primary_package & 128 ? " per die" : "");
    do_line ("Chip Organisation", linebuf);
    if (eeprom->secondary_package) {
        snprintf (linebuf, sizeof (linebuf) - 1,
                  "%s, %sBitx%d%s",
                  get_ddr4_package (eeprom->secondary_package),
                  mtostr (get_ddr4_secondary_density (eeprom->density_banks, eeprom->secondary_package)),
                  4 << (eeprom->organization & 7),
                  eeprom->primary_package & 128 ? " per die" : "");
        do_line ("Secondary Organisation", linebuf);
    }

    /* primary timings */
    if (eeprom->timebases) {
        printf ("Unknown timebases, not calculating timing information");
        return;
    }
    mtb = 125; /* in ps */
    ftb = 1;   /* in ps */

    /* Tckavg - minimum and maximum cycle time, converts to clock frequency */
    int tckavgmin = mtb * eeprom->min_tckavg + ftb * eeprom->fine_min_tckavg;
    int tckavgmax = mtb * eeprom->max_tckavg + ftb * eeprom->fine_max_tckavg;
    double maxfck = 1000000.0 / (double)tckavgmin;
    double minfck = 1000000.0 / (double)tckavgmax;
    snprintf (linebuf, sizeof (linebuf) - 1, "%.0lfMHz - %.0lfMhz (%dps - %dps period)",
              minfck, maxfck, tckavgmax, tckavgmin);
    do_line ("Clock Range", linebuf);

    /* CAS Latencies */
    int cls[30];
    int cl, clcount = 0;
    if (eeprom->cas_latencies[3] & 128) cl = 23;
    else cl = 7;
    for (i=0; i<30; i++, cl++)
        if (eeprom->cas_latencies[i>>3] & (1 << (i & 7)))
            cls[clcount++] = cl;
    linebuf2[0] = 0;
    for (i=0; i<clcount; i++) {
        sprintf (linebuf, "%d", cls[i]);
        addlist (linebuf2, linebuf);
    }
    do_line ("Valid CL values", linebuf2);

    int taamin  = mtb * eeprom->min_taa  + ftb * eeprom->fine_min_taa;
    int trcdmin = mtb * eeprom->min_trcd + ftb * eeprom->fine_min_trcd;
    int trrpmin = mtb * eeprom->min_trp  + ftb * eeprom->fine_min_trp;
    int trasmin = mtb * ((((int)eeprom->min_tras_trc_upper & 15) << 8) + eeprom->min_tras_lower);

    snprintf (linebuf, sizeof (linebuf) - 1,
              "tAA: %gns tRCD: %gns tRP: %gns tRAS: %gns",
              (double)taamin  / 1000.0,  (double)trcdmin / 1000.0,
              (double)trrpmin / 1000.0,  (double)trasmin / 1000.0);
    do_line ("Minimum pri timings", linebuf);


    for (i=0; i<num_ddr4_frequencies; i++)
        if (ddr4_frequencies[i] >= minfck && ddr4_frequencies[i] <= maxfck) {
            sprintf (linebuf, "Latencies at %dMHz", ddr4_frequencies[i]);
            sprintf (linebuf2, "%d-%d-%d-%d",
                     /* cl   */ ps_to_clocks (taamin,  ddr4_periods[i]),
                     /* trcd */ ps_to_clocks (trcdmin, ddr4_periods[i]),
                     /* trrp */ ps_to_clocks (trrpmin, ddr4_periods[i]),
                     /* tras */ ps_to_clocks (trasmin, ddr4_periods[i]));
            do_line (linebuf, linebuf2);
        }

    do_line (NULL, NULL);
}
