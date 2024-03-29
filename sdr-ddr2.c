#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vendors.h"
#include "constants.h"
#include "struct.h"
#include "output.h"
#include "sdr-ddr2.h"

static char *get_ddr2_memtype (const char type) {
    switch (type) {
        case DDR2MODULETYPE_UNDEFINED:    return "unspecified module type";
        case DDR2MODULETYPE_RDIMM:        return "RDIMM";
        case DDR2MODULETYPE_UDIMM:        return "UDIMM";
        case DDR2MODULETYPE_SO_DIMM:      return "SO-DIMM";
        case DDR2MODULETYPE_72B_SO_CDIMM: return "72b-SO-CDIMM";
        case DDR2MODULETYPE_72B_SO_RDIMM: return "72b-SO-RDIMM";
        case DDR2MODULETYPE_MICRO_DIMM:   return "Micro-DIMM";
        case DDR2MODULETYPE_MINI_RDIMM:   return "Mini-RDIMM";
        case DDR2MODULETYPE_MINI_UDIMM:   return "Mini-UDIMM";
    }
    return "invalid module type";
}

static int crc (const char *data, int count) {
    int crc, i;
    crc = 0;

    for (i = 0; i < count; i++)
        crc += *data++;

    return crc & 0xFF;
}

static int latency (int memtype, double cyclen, int value) {
    switch (memtype) {
    case MEMTYPE_SDR:
        return (int) ceil ((double) value / cyclen);
    case MEMTYPE_DDR:
    case MEMTYPE_DDR2:
        return (int) ceil ((double) value / 4.0 / cyclen);
    }
    return -1;
}

void do_sdram (const struct sdram_spd *eeprom, int length) {
    int i, checksum;

    int rows[MAX_RANKS], columns[MAX_RANKS];
    int width;
    int size = 0;
    int num_ranks;

    int max_cl = 7;
    double cyclen;
    int cl, cyclen_i;

    char linebuf[256], linebuf2[200], cls[16];

    if ((length < 2) || (length < (1 << eeprom->total_bytes))) {
        printf ("Insufficient data read, aborting decode\n");
        return;
    }

    /* SPD information */
    switch (eeprom->memory_type) {
    case MEMTYPE_SDR:
        sprintf (linebuf, "%d", eeprom->spd_revision);
        break;
    case MEMTYPE_DDR:
    case MEMTYPE_DDR2:
        sprintf (linebuf, "%d.%d", eeprom->spd_revision >> 4,
                 eeprom->spd_revision & 15);
        break;
    }
    sprintf (linebuf2, "%d/%d bytes used",
             eeprom->bytes_written, 1 << eeprom->total_bytes);
    strcat (linebuf, linebuf2);
    do_line ("SPD Revision", linebuf);
    checksum = crc ((char *) eeprom, 63);
    sprintf (linebuf, "%02X, %scorrect", checksum,
             checksum == eeprom->checksum ? "" : "not ");
    do_line ("Checksum", linebuf);

    /* Vendor information */
    if (eeprom->bytes_written >= 71) {
        do_line ("Vendor", get_vendor64 (eeprom->manufacturer_jedec_id));
        if (eeprom->bytes_written >= 90) {
            memcpy (linebuf, &(eeprom->part_number), 18);
            linebuf[18] = 0;
            for (i = 17; i && linebuf[i] == -1; i--)
                linebuf[i] = 0;
            do_line ("Part Number", linebuf);
        }
    }
    else
        do_line ("Vendor", "unknown");

    /* general module type */
    num_ranks = eeprom->num_ranks & 7;
    if (eeprom->memory_type == MEMTYPE_DDR2)
        num_ranks++;
    if (num_ranks > MAX_RANKS) {
        do_error ("update decode-dimm to support a minimum of %d ranks\n",
                  num_ranks);
        return;
    }
    rows[0] = eeprom->num_row_addr & 15;
    if (eeprom->memory_type < MEMTYPE_DDR2)
        rows[1] = eeprom->num_row_addr >> 4;
    else
        rows[1] = 0;
    width = eeprom->data_width + eeprom->reserved1 * 256;
    for (i = 1; i < MAX_RANKS; i++)
        if (!rows[i])
            rows[i] = rows[0];
    for (i = 0; i < MAX_RANKS; i++)
        if (rows[i] < 7)
            rows[i] += 15;

    columns[0] = eeprom->num_col_addr & 15;
    if (eeprom->memory_type < MEMTYPE_DDR2)
        columns[1] = eeprom->num_col_addr >> 4;
    else
        columns[1] = 0;
    for (i = 0; i < MAX_RANKS; i++) {
        if (i && !columns[i])
            columns[i] = columns[0];
        if (columns[i] < 7)
            columns[i] += 15;
    }

    for (i = 0; i < num_ranks; i++)
        size +=
            (1 << (rows[i] + columns[i] - 20)) * eeprom->num_banks_device *
            (width >> 3);

    switch (eeprom->memory_type) {
    case MEMTYPE_SDR:
        strcpy (linebuf2, "SDR SDRAM");
        break;
    case MEMTYPE_DDR:
        strcpy (linebuf2, "DDR SDRAM");
        break;
    case MEMTYPE_DDR2:
        strcpy (linebuf2, "DDR2 ");
        strcat (linebuf2, get_ddr2_memtype (eeprom->dimm_type));
        break;
    }

    switch (eeprom->memory_type) {
    case MEMTYPE_SDR:
    case MEMTYPE_DDR:
        if (eeprom->module_attr & ATTR_DDR_BUFFERED)
            strcat (linebuf2, " buffered");
        if (eeprom->module_attr & ATTR_DDR_REGISTERED)
            strcat (linebuf2, " registered");
        break;
    case MEMTYPE_DDR2:
        if (eeprom->dimm_type == DDR2MODULETYPE_RDIMM ||
            eeprom->dimm_type == DDR2MODULETYPE_72B_SO_RDIMM ||
            eeprom->dimm_type == DDR2MODULETYPE_MINI_RDIMM)
            strcat (linebuf2, " registered");
        break;
    }

    if ((eeprom->config_type & CONFIG_DATA_PARITY) &&
        !(eeprom->config_type & CONFIG_DATA_ECC)) {
        strcat (linebuf2, " parity");
        size = ceil ((double) size * 8.0 / 9.0);
    }
    else if (eeprom->config_type & CONFIG_DATA_ECC) {
        strcat (linebuf2, " data ECC");
        size = ceil ((double) size * 8.0 / 9.0);
    }
    if (eeprom->config_type & CONFIG_ADDR_PARITY)
        strcat (linebuf2, " address/command parity");

    snprintf (linebuf, sizeof (linebuf) - 1, "%100s %dMB", linebuf2, size);
    do_line ("Part Type", linebuf);

    /* organisation */
    snprintf (linebuf2, sizeof (linebuf2) - 1, "%d rank%s, %d bank%s%s,",
              num_ranks, num_ranks == 1 ? "" : "s",
              eeprom->num_banks_device,
              eeprom->num_banks_device == 1 ? "" : "s",
              num_ranks > 1 ? " each" : "");
    for (i = 0; i < num_ranks; i++) {
        snprintf (linebuf, sizeof (linebuf),
                  "%s %d rows/%d columns (%dMBitx%d)", i ? "and" : linebuf2,
                  rows[i], columns[i],
                  (1 << (rows[i] + columns[i] - 20)) *
                  eeprom->num_banks_device, width);
        if (!i || (i && (rows[i] != rows[0] || columns[i] != columns[0])))
            do_line (i ? "" : "Organisation", linebuf);
    }

    /* timing */
    for (cl = 6; cl >= 0; cl--)
        if (eeprom->cas_latency & (1 << cl)) {
            if (max_cl == 7)
                max_cl = cl;
            if (cl == max_cl)
                cyclen_i = eeprom->min_clk_cycle_cl_max_0;
            else if (cl == max_cl - 1)
                cyclen_i = eeprom->min_clk_cycle_cl_max_1;
            else if (cl == max_cl - 2)
                cyclen_i = eeprom->min_clk_cycle_cl_max_2;
            else
                break;

            if ((cyclen_i & 15) < 10) {
                cyclen = (double) (cyclen_i & 15) / 10.0;
            }
            else {
                switch (cyclen_i & 15) {
                case 0x0a:
                    cyclen = 0.25;
                    break;
                case 0x0b:
                    cyclen = 1.0 / 3.0;
                    break;
                case 0x0c:
                    cyclen = 2.0 / 3.0;
                    break;
                case 0x0d:
                    cyclen = 0.75;
                    break;
                default:
                    cyclen = 0.0;
                }
            }
            cyclen += (double) (cyclen_i >> 4);

            switch (eeprom->memory_type) {
            case MEMTYPE_SDR:
                sprintf (cls, "%d", 1 + cl);
                break;
            case MEMTYPE_DDR:
                sprintf (cls, "%3.1f", 1 + (double) cl * 0.5);
                break;
            case MEMTYPE_DDR2:
                sprintf (cls, "%d", cl);
                break;
            }
            sprintf (linebuf2, "%s-%d-%d-%d",
                     /* cl */   cls,
                     /* trcd */ latency (eeprom->memory_type, cyclen, eeprom->min_trcd),
                     /* trp */  latency (eeprom->memory_type, cyclen, eeprom->min_trp),
                     /* tras */ (int) ceil ((double) eeprom->min_tras / cyclen));
            sprintf (linebuf, "Latencies at %dMHz", (int) (1000.0 / cyclen));
            do_line (linebuf, linebuf2);
        }
}
