#include "i2c-tools-i2c-dev.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <sys/ioctl.h>

#include "vendors.h"
#include "constants.h"
#include "struct.h"

#define I2C_START "moo"

void do_line (const char * description, const char * content) {
  printf ("%-19s%c %s\n", description, description[0]?':':' ', content);
}

const char * get_vendor (const unsigned char * vendor_id) {
  int i, j;

  for (i=0; jedec_vendors[i].name; i++) {
    for (j=0; j<8; j++) {
      if (vendor_id[j] != jedec_vendors[i].id[j])
	break;
      if (vendor_id[j] != 0x7F)
	return jedec_vendors[i].name;
    }
  }
  return "unknown";
}

const char * get_vendor2 (const u16 vendor_id) {
  unsigned char vendor_id_long[8];
  int i;

  bzero (vendor_id_long, sizeof (vendor_id_long));
  for (i=0; i < (vendor_id & 127); i++)
    vendor_id_long[i] = 0x7F;
  vendor_id_long[vendor_id & 127] = vendor_id >> 8;

  return get_vendor (vendor_id_long);
}

int latency (int memtype, double cyclen, int value) {
  switch (memtype) {
  case MEMTYPE_SDR: 
    return (int) ceil ((double) value / cyclen);
  case MEMTYPE_DDRSDR:
  case MEMTYPE_DDR2SDR:
    return (int) ceil ((double) value / 4.0 / cyclen);
  }
  return -1;
}

void do_sdram (const sdram_spd_t * eeprom) {
  int i;

  int rows[MAX_RANKS], columns[MAX_RANKS];
  int width;
  int size = 0;
  int num_ranks;

  int max_cl = 7;
  double cyclen;
  int cl, cyclen_i;

  char linebuf[256], linebuf2[256];

  /* Vendor information */
  if (eeprom->bytes_written >= 71) {
    do_line ("Vendor", get_vendor (eeprom->manufacturer_jedec_id));
    if (eeprom->bytes_written >= 90) {
      memcpy (linebuf, &(eeprom->part_number), 18);
      linebuf[18] = 0;
      for (i=17; i && linebuf[i] == -1; i--)
	linebuf[i] = 0;
      do_line ("Part Number", linebuf);
    }
  } else do_line ("Vendor", "unknown");

  /* general module type */
  num_ranks = eeprom->num_ranks & 7;
  if (eeprom->memory_type == MEMTYPE_DDR2SDR) num_ranks++;
  if (num_ranks > MAX_RANKS) {
    printf ("update decode-dimm to support a minimum of %d ranks\n", 
	    num_ranks);
    return;
  }
  rows[0] = eeprom->num_row_addr & 15;
  if (eeprom->memory_type < MEMTYPE_DDR2SDR) rows[1] = eeprom->num_row_addr >> 4;
  else rows[1] = 0;
  width = eeprom->data_width + eeprom->reserved1 * 256;
  for (i=1; i<MAX_RANKS; i++)
    if (!rows[i]) rows[i] = rows[0];
  for (i=0; i<MAX_RANKS; i++)
    if (rows[i] < 7) rows[i] += 15;

  columns[0] = eeprom->num_col_addr & 15;
  if (eeprom->memory_type < MEMTYPE_DDR2SDR) columns[1] = eeprom->num_col_addr >> 4;
  else columns[1] = 0;
  for (i=0; i<MAX_RANKS; i++) {
    if (i && !columns[i]) columns[i] = columns[0];
    if (columns[i] < 7) columns[i] += 15;
  }

  for (i=0; i<num_ranks; i++) 
    size += (1 << (rows[i] + columns[i] - 20)) * eeprom->num_banks_device * 
      (width >> 3);

  switch (eeprom->memory_type) {
  case MEMTYPE_SDR: strcpy (linebuf, "SDR SDRAM"); break;
  case MEMTYPE_DDRSDR: strcpy (linebuf, "DDR SDRAM"); break;
  case MEMTYPE_DDR2SDR: strcpy (linebuf, "DDR2 SDRAM"); break;
  }

  switch (eeprom->memory_type) {
  case MEMTYPE_SDR: 
  case MEMTYPE_DDRSDR:
    if (eeprom->module_attr & ATTR_DDR_BUFFERED) strcat (linebuf, " buffered");
    if (eeprom->module_attr & ATTR_DDR_REGISTERED) strcat (linebuf, " registered");
    break;
  case MEMTYPE_DDR2SDR:
    if (eeprom->module_attr & ATTR_DDR2_REGISTERED) strcat (linebuf, " registered");
    break;
  }

  if ((eeprom->config_type & CONFIG_DATA_PARITY) &&
      !(eeprom->config_type & CONFIG_DATA_ECC)) {
    strcat (linebuf, " parity"); 
    size = ceil ((double) size * 8.0 / 9.0);
  } else if (eeprom->config_type & CONFIG_DATA_ECC) {
    strcat (linebuf, " data ECC");
    size = ceil ((double) size * 8.0 / 9.0);
  }
  if (eeprom->config_type & CONFIG_ADDR_PARITY)
    strcat (linebuf, " address/command parity");

  snprintf (linebuf2, 256, "%s %dMB", linebuf, size);
  do_line ("Part Type", linebuf2);

  /* organisation */
  snprintf (linebuf2, 256, "%d rank%s, %d bank%s%s,", 
	    num_ranks, num_ranks == 1 ? "" : "s",
	    eeprom->num_banks_device, eeprom->num_banks_device == 1 ? "" : "s",
	    num_ranks > 1 ? " each" : "");
  for (i=0; i<num_ranks; i++) {
    snprintf (linebuf, 256, "%s %d rows/%d columns (%dMBitx%d)",
	      i ? "and" : linebuf2, rows[i], columns[i],
	      (1 << (rows[i] + columns[i] - 20)) * eeprom->num_banks_device, 
	      width);
    if (!i || (i && (rows[i] != rows[0] || columns[i] != columns[0])))
      do_line (i ? "" : "Organisation", linebuf);
  }

  /* timing */
  for (cl = 6; cl >= 0; cl--) if (eeprom->cas_latency & (1 << cl)) {
    if (max_cl == 7) max_cl = cl;
    if (cl == max_cl) cyclen_i = eeprom->min_clk_cycle_cl_max_0;
    else if (cl == max_cl - 1) cyclen_i = eeprom->min_clk_cycle_cl_max_1;
    else if (cl == max_cl - 2) cyclen_i = eeprom->min_clk_cycle_cl_max_2;
    else break;

    if ((cyclen_i & 15) < 10) {
      cyclen = (double) (cyclen_i & 15) / 10.0;
    } else {
      switch (cyclen_i & 15) {
      case 0x0a: cyclen = 0.25; break;
      case 0x0b: cyclen = 1.0 / 3.0; break;
      case 0x0c: cyclen = 2.0 / 3.0; break;
      case 0x0d: cyclen = 0.75; break;
      default: cyclen = 0.0;
      }
    }
    cyclen += (double) (cyclen_i >> 4);

    switch (eeprom->memory_type) {
    case MEMTYPE_SDR: sprintf (linebuf, "%d", 1+cl); break;
    case MEMTYPE_DDRSDR: sprintf (linebuf, "%3.1f", 1+(double) cl * 0.5); break;
    case MEMTYPE_DDR2SDR: sprintf (linebuf, "%d", cl); break;
    }
    sprintf (linebuf2, "%s-%d-%d-%d", 
	     /* cl */   linebuf,
	     /* trcd */ latency (eeprom->memory_type, cyclen, eeprom->min_trcd),
	     /* trp */  latency (eeprom->memory_type, cyclen, eeprom->min_trp),
	     /* tras */ (int) ceil ((double) eeprom->min_tras / cyclen));
    sprintf (linebuf, "Latencies at %dMHz", (int) (1000.0 / cyclen));
    do_line (linebuf, linebuf2);
  }

  printf ("\n");
}

int ddr3_crc (const ddr3_sdram_spd_t * eeprom) {
  int count, i, crc = 0;
  char * eeprom_ = (char *) eeprom;

  count = (eeprom->bytes_used & 0x80) ? 117 : 126;

  while (--count >= 0) {
    crc = crc ^ (int)*eeprom_++ << 8;
    for (i = 0; i < 8; ++i)
      if (crc & 0x8000) crc = crc << 1 ^ 0x1021;
      else crc = crc << 1;
  }
  crc &= 0xffff;

  return eeprom->crc == crc;
}

void do_ddr3 (const ddr3_sdram_spd_t * eeprom) {
  int i;
  int rows, columns, banks, ranks;
  int width, size;
  int min_tras;
  double  mtb;
  char linebuf[256], linebuf2[256];

  const int ddr3_frequencies[] = { 1600, 1333, 1066, 1000, 933, 900, 800, 667, 533, 400 };
  const int num_ddr3_frequencies = sizeof (ddr3_frequencies) / sizeof (ddr3_frequencies[0]);

  inline int get_cl (int freq) {
    int i, cl;

    cl = ceil ((eeprom->min_taa*mtb) * freq / 1000.0);
    for (i = cl; i < 15; i++)
      if (eeprom->cas_latency & (1 << (i-4)))
	return i;
    return -1;
  }

  /* Vendor information */
  do_line ("Module Vendor", get_vendor2 (eeprom->manufacturer_jedec_id));
  if ((eeprom->bytes_used & 15) > 1) {
    if (eeprom->dram_manufacturer_jedec_id)
      do_line ("Chip Vendor", get_vendor2 (eeprom->dram_manufacturer_jedec_id));
    memcpy (linebuf, &(eeprom->part_number), 18);
    linebuf[18] = 0;
    do_line ("Part Number", linebuf);
  }

  /* general module type */
  ranks = ((eeprom->organization >> 3) & 7) + 1;
  if (ranks > MAX_RANKS) {
    printf ("update decode-dimm to support a minimum of %d ranks\n", ranks);
    return;
  }

  rows = ((eeprom->adressing >> 3) & 7) + 12;
  columns = (eeprom->adressing & 7) + 9;
  banks = 8 << ((eeprom->density_banks >> 3) & 7 );
  width = (8 << (eeprom->bus_width & 7)) + (((eeprom->bus_width >> 3) & 3) == 1 ? 8 : 0);
  size = ranks * (1 << (rows + columns - 20)) * banks * ((width >> 3) & 0xfc);

  strcpy (linebuf, "DDR3 SDRAM");
  if ((eeprom->module_type & 15) == DDR3MODULETYPE_RDIMM ||
      (eeprom->module_type & 15) == DDR3MODULETYPE_MINI_RDIMM)
    strcat (linebuf, " REGISTERED");
  if (((eeprom->bus_width >> 3) & 3) == 1)
    strcat (linebuf, " ECC");

  snprintf (linebuf2, 256, "%s %dMB", linebuf, size);
  do_line ("Part Type", linebuf2);

  /* organisation */
  snprintf (linebuf, 256, "%d rank%s, %d bank%s%s, %d rows/%d columns (%dMBitx%d)",
	    ranks, ranks == 1 ? "" : "s", banks, banks == 1 ? "" : "s", ranks > 1 ? " each" : "",
	    rows, columns, (1 << (rows + columns - 20)) * banks, width);
  do_line ("Organisation", linebuf);


  do_line ("CRC", ddr3_crc (eeprom) ? "matches" : "does not match");

  /* timing */
  mtb = (float) eeprom->mtb_dividend / (float) eeprom->mtb_divisor;
  min_tras = eeprom->min_tras_lsb + ((int) (eeprom->min_tras_trc_upper_nibble & 15) << 8);

  for (i=0; i<num_ddr3_frequencies && ddr3_frequencies[i] > (int) (1000/(eeprom->min_tck * mtb)); i++) ;
  for (; i<num_ddr3_frequencies; i++) {
    sprintf (linebuf2, "%d-%d-%d-%d", 
	     /* cl   */ get_cl (ddr3_frequencies[i]),
	     /* trcd */ (int) ceil (eeprom->min_trcd * mtb * ddr3_frequencies[i] / 1000.0),
	     /* trp  */ (int) ceil (eeprom->min_trp * mtb * ddr3_frequencies[i] / 1000.0),
	     /* tras */ (int) ceil (min_tras * mtb * ddr3_frequencies[i] / 1000.0));

    sprintf (linebuf, "Latencies at %dMHz", ddr3_frequencies[i]);
    do_line (linebuf, linebuf2);
  }

  printf ("\n");
}

int do_eeprom (int device, const unsigned char * eeprom) {
  int i, j;

  printf ("Analyzing client 0x%02x (Probably slot %d)\n", device, device - 0x4f);
  switch (eeprom[2]) {
  case MEMTYPE_SDR: do_sdram ((sdram_spd_t *) eeprom); break;
  case MEMTYPE_DDRSDR: do_sdram ((sdram_spd_t *) eeprom); break;
  case MEMTYPE_DDR2SDR: do_sdram ((sdram_spd_t *) eeprom); break;
  case MEMTYPE_DDR3SDR: do_ddr3 ((ddr3_sdram_spd_t *) eeprom); break;
/*  case 0xff:
    if (eeprom[0]==0x00 && eeprom[1]==0xff && eeprom[2]==0xff && eeprom[3]==0xff &&
	eeprom[4]==0xff && eeprom[5]==0xff && eeprom[6]==0xff && eeprom[7]==0x00) {
	    printf ("EDID eeprom, skipping bus\n");
	return -2;
    }
  */default:
    for (i=0; i<32; i++)
      for (j=0; j<8; j++)
        printf ("0x%02x,%s", eeprom[8*i+j], j<7?" ":"\n");
    printf ("Unsupported memory type %d\n", eeprom[2]);
    return -1;
  }
  return 0;
}

void do_bus (const char * name) {
  char full_name [256];
  struct stat statbuf;
  int result, features, retry;
  int adapter, client, address;
  unsigned char eeprom[256];
  int has_word;

  snprintf (full_name, 256, "/dev/%s", name);
  if ((result = stat (full_name, &statbuf))) {
    fprintf (stderr, "Can't stat() %s: %s\n", full_name, strerror (errno));
    return;
  }
  if (!S_ISCHR(statbuf.st_mode)) {
    fprintf (stderr, "%s is not a character device.\n", name);
    return;
  }

  adapter = open (full_name, O_RDWR);
  if (adapter < 0) {
    fprintf (stderr, "Can't open %s: %s\n", full_name, strerror (errno));
    return;
  }
  printf ("Probing %s\n", name);

  if (ioctl (adapter, I2C_FUNCS, &features)) {
    fprintf (stderr, "Can't retrieve features: %s\n", strerror (errno));
    return;
  }

  if (!(features & (I2C_FUNC_SMBUS_READ_BYTE_DATA | I2C_FUNC_SMBUS_READ_WORD_DATA))) {
    fprintf (stderr, "No access to the driver");
    return;
  }

  if ((features & I2C_FUNC_SMBUS_PEC) && ioctl (adapter, I2C_PEC, 0)) {
    fprintf (stderr, "Can't disable PEC: %s\n", strerror (errno));
    return;
  }
  if ((features & I2C_FUNC_10BIT_ADDR) && ioctl (adapter, I2C_TENBIT, 0)) {
    fprintf (stderr, "Can't disable tenbit: %s\n", strerror (errno));
    return;
  }

  for (client = 0x50; client < 0x58; client++) {
    if (ioctl (adapter, I2C_SLAVE_FORCE, client)) {
      fprintf (stderr, "Can't select client 0x%02x: %s\n", client, strerror (errno));
      continue;
    }

    has_word = (features & I2C_FUNC_SMBUS_READ_WORD_DATA) == I2C_FUNC_SMBUS_READ_WORD_DATA;
    for (address = 0; address < 256; address+=1+has_word) {
      retry = 0;
      do {
        retry++;
        if (has_word) result = i2c_smbus_read_word_data (adapter, address);
        else result = i2c_smbus_read_byte_data (adapter, address);
      } while (result < 0 && retry < 5);
      if (result < 0)
        break;
      eeprom[address] = result & 0xff;
      if (has_word) eeprom[address+1] = (result >> 8) & 0xff;
    }
    if (result >= 0)
      if (do_eeprom (client, eeprom) == -2)
        break;
  }
  close (adapter);
}

int main () {
  struct stat statbuf;
  int result;
  DIR * dev;
  struct dirent * device;

  if ((result = stat ("/dev", &statbuf))) {
    fprintf (stderr, "Can't stat() /dev: %s\n", strerror (errno));
    return 1;
  }
  if (!S_ISDIR(statbuf.st_mode)) {
    fprintf (stderr, "/dev is not a directory.\n");
    return 1;
  }

  if (!(dev = opendir ("/dev"))) {
    fprintf (stderr, "Can't opendir /dev: %s\n", strerror (errno));
    return 1;
  }

  while ((device = readdir (dev)))
    if (device->d_name == strstr (device->d_name, "i2c-"))
	do_bus (device->d_name);

  closedir (dev);

  return 0;
}
