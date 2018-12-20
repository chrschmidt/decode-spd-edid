#include <math.h>
#include <string.h>
#include <stdio.h>
#include "constants.h"
#include "struct.h"
#include "output.h"
#include "vendors.h"
#include "ddr3.h"

static const char *moduletypenames[] = {
    "Undefined", "RDIMM", "UDIMM", "SO-DIMM",
    "MICRO-DIMM", "Mini-RDIMM", "Mini-UDIMM", "Mini-CDIMM",
    "72b-SO-UDIMM", "72b-SO-RDIMM", "72b-SO-CDIMM", "LRDIMM",
    "16b-SO-DIMM", "32b-SO-DIMM"
};

static void do_xmp_profile (const struct ddr3_xmp_profile *profile,
                            const char *name, const double mtb) {
    char linebuf[256], linebuf2[256];
    double freq = 1000 / (profile->min_tck * mtb);
    int cl, i, min_tras;

    snprintf (linebuf, sizeof (linebuf) - 1, "%s profile", name);
    cl = ceil (profile->min_taa / profile->min_tck);
    for (i = cl; i < 15; i++)
        if (profile->cas_latency & (1 << (i - 4))) {
            cl = i;
            break;
        }
    min_tras = profile->min_tras_lsb +
               ((int) (profile->min_tras_trc_upper_nibble & 15) << 8);

    sprintf (linebuf2, "%dMHz at %d.%dV, latencies %d-%d-%d-%d",
             (int) round (freq),
             (profile->voltage > 5) & 3, (profile->voltage & 31) * 5,
             /* cl   */ cl,
             /* trcd */ (int) ceil (profile->min_trcd / profile->min_tck),
             /* trp  */ (int) ceil (profile->min_trp / profile->min_tck),
             /* tras */ (int) ceil (min_tras / profile->min_tck));

    do_line (linebuf, linebuf2);
}

static void do_xmp (const struct ddr3_xmp *xmp) {
    char linebuf[256];

    sprintf (linebuf, "XMP revision %d.%d",
             xmp->revision >> 4, xmp->revision & 15);
    do_line ("", linebuf);

    if (xmp->profile_org_conf & 1)
        do_xmp_profile (&xmp->profiles[0], "Certified",
                        (double) xmp->p1_mtb_dividend / (double) xmp->p1_mtb_divisor);
    if (xmp->profile_org_conf & 2)
        do_xmp_profile (&xmp->profiles[1], "Extreme",
                        (double) xmp->p2_mtb_dividend / (double) xmp->p2_mtb_divisor);
}

static int ddr3_bytesused (const unsigned char x) {
    switch (x & 15) {
    case 0:
        return 0;
    case 1:
        return 128;
    case 2:
        return 176;
    case 3:
        return 256;
    default:
        return -1;
    }
}

static int ddr3_bytestotal (const unsigned char x) {
    switch ((x >> 4) & 7) {
    case 0:
        return 0;
    case 1:
        return 256;
    default:
        return -1;
    }
}

static int ddr3_crc (const char *data, int count) {
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

void do_ddr3 (const struct ddr3_sdram_spd *eeprom) {
    int i;
    int rows, columns, banks, ranks;
    int width, size;
    int min_tras;
    double mtb, freq;
    char linebuf[200], linebuf2[256];
    int checksum;

    const int ddr3_frequencies[] =
        { 1500, 1466, 1400, 1333, 1200, 1066, 1000, 933, 900, 800, 667, 533, 400 };
    const int num_ddr3_frequencies =
        sizeof (ddr3_frequencies) / sizeof (ddr3_frequencies[0]);

    inline int get_cl (int freq) {
        int i, cl;

        cl = ceil ((eeprom->min_taa * mtb) * freq / 1000.0);
        for (i = cl; i < 15; i++)
            if (eeprom->cas_latency & (1 << (i - 4)))
                return i;
        return -1;
    }

    /* SPD information */
    sprintf (linebuf, "%d.%d", eeprom->spd_revision >> 4,
             eeprom->spd_revision & 15);
    if (eeprom->bytes_used_crc & 15 && eeprom->bytes_used_crc & 112) {
        sprintf (linebuf2, ", %d/%d bytes used",
                 ddr3_bytesused (eeprom->bytes_used_crc),
                 ddr3_bytestotal (eeprom->bytes_used_crc));
        strcat (linebuf, linebuf2);
    }
    do_line ("SPD Revision:", linebuf);
    checksum = ddr3_crc ((char *) eeprom,
                         (eeprom->bytes_used_crc & 128) ? 117 : 126);
    sprintf (linebuf, "04%X, %scorrect", checksum,
             checksum == eeprom->crc ? "" : "not ");
    do_line ("Checksum", linebuf);

    /* Vendor information */
    sprintf (linebuf, "%s (%04x)",
             get_vendor16 (eeprom->manufacturer_jedec_id),
             eeprom->manufacturer_jedec_id);
    do_line ("Module Vendor", linebuf);
    if (ddr3_bytesused (eeprom->bytes_used_crc) > 128) {
        if (eeprom->dram_manufacturer_jedec_id) {
            sprintf (linebuf, "%s (%04x)",
                     get_vendor16 (eeprom->dram_manufacturer_jedec_id),
                     eeprom->dram_manufacturer_jedec_id);
            do_line ("Chip Vendor", linebuf);
        }
        memcpy (linebuf, &(eeprom->part_number), 18);
        linebuf[18] = 0;
        do_line ("Part Number", linebuf);
    }

    /* general module type */
    ranks = ((eeprom->organization >> 3) & 7) + 1;
    if (ranks > MAX_RANKS) {
        do_error ("update decode-dimm to support a minimum of %d ranks\n",
                  ranks);
        return;
    }

    rows = ((eeprom->adressing >> 3) & 7) + 12;
    columns = (eeprom->adressing & 7) + 9;
    banks = 8 << ((eeprom->density_banks >> 3) & 7);
    width = (8 << (eeprom->bus_width & 7)) +
            (((eeprom->bus_width >> 3) & 3) == 1 ? 8 : 0);
    size = ranks * (1 << (rows + columns - 20)) * banks * (width >> 3);

    strcpy (linebuf, "DDR3 ");
    if (eeprom->module_type & 15)
        strcat (linebuf, moduletypenames[eeprom->module_type & 15]);

    if (((eeprom->bus_width >> 3) & 3) == 1)
        snprintf (linebuf2, sizeof (linebuf2) - 1, "%s (ECC) %d/%dMB",
                  linebuf, size * 8 / 9, size);
    else
        snprintf (linebuf2, sizeof (linebuf2) - 1, "%s %dMB", linebuf, size);
    do_line ("Part Type", linebuf2);

    /* voltage */
    linebuf[0] = 0;
    if (eeprom->voltage & 4)
        addlist (linebuf, "1.2V");
    if (eeprom->voltage & 2)
        addlist (linebuf, "1.35V");
    if (!(eeprom->voltage & 1))
        addlist (linebuf, "1.5V");
    do_line ("Voltage", linebuf);

    /* organisation */
    snprintf (linebuf, 256,
              "%d rank%s, %d bank%s%s, %d rows/%d columns (%dMBitx%d)", ranks,
              ranks == 1 ? "" : "s", banks, banks == 1 ? "" : "s",
              ranks > 1 ? " each" : "", columns, rows,
              (1 << (rows + columns - 20)) * banks, width);
    do_line ("Organisation", linebuf);

    /* timing */
    mtb = (double) eeprom->mtb_dividend / (double) eeprom->mtb_divisor;
    freq = 1000 / (eeprom->min_tck * mtb);
    min_tras = eeprom->min_tras_lsb +
               ((int) (eeprom->min_tras_trc_upper_nibble & 15) << 8);

    for (i = 0;
         i < num_ddr3_frequencies && ddr3_frequencies[i] > (int) round (freq);
         i++);
    for (; i < num_ddr3_frequencies; i++) {
        sprintf (linebuf2, "%d-%d-%d-%d",
                 /* cl   */ get_cl (ddr3_frequencies[i]),
                 /* trcd */ (int) ceil (eeprom->min_trcd * mtb * ddr3_frequencies[i] / 1000.0),
                 /* trp  */ (int) ceil (eeprom->min_trp * mtb * ddr3_frequencies[i] / 1000.0),
                 /* tras */ (int) ceil (min_tras * mtb * ddr3_frequencies[i] / 1000.0));
        sprintf (linebuf, "Latencies at %dMHz", ddr3_frequencies[i]);
        do_line (linebuf, linebuf2);
    }
    if (eeprom->xmp.id == 0x4a0c)
        do_xmp (&eeprom->xmp);
    do_line (NULL, NULL);
}
