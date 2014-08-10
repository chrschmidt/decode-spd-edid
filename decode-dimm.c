#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#include "vendors.h"
#include "constants.h"
#include "struct.h"

#define I2C_START "/sys/bus/i2c/devices"

char * get_i2c_bus_name (const char * id) {
  char bus [256];
  char busname [256];
  struct stat statbuf;
  FILE * file;

  snprintf (bus, sizeof (bus), "/sys/class/i2c-adapter/i2c-%s/device/name", id);
  if (stat (bus, &statbuf)) {
    snprintf (bus, sizeof (bus), "/sys/class/i2c-adapter/i2c-%s/name", id);
    if (stat (bus, &statbuf))
      return NULL;
  }

  busname[0] = 0;
  if (!(file = fopen (bus, "r")))
    return NULL;
  fgets (busname, sizeof (busname), file);
  fclose (file);
  busname [strlen(busname)-1] = 0;

  return strdup (busname);
}

void do_eeprom (const char * full_path, char * eeprom) {
  char device [7];
  int bank;

  strncpy (device, full_path+strlen(I2C_START)+1, 6);
  device[6] = 0;
  bank = atoi (full_path+strlen(I2C_START)+5) - 49;

  printf ("Analyzing %s (Probably slot %d)\n", device, bank);
}

int do_eeprom_from_file (char * filename) {
  char eeprom[256];
  FILE * file;
  char * bus, * device, *busname;
  int result;

  if (!(file = fopen (filename, "r")))
    return -1;
  result = fread (&eeprom, 1, sizeof (eeprom), file);
  fclose (file);
  if (result != sizeof (eeprom))
    return -1;

  *strrchr (filename, '/') = 0;
  bus = strrchr (filename, '/') + 1;
  device = strchr (bus, '-');
  *device++ = 0;
  busname = get_i2c_bus_name (bus);
  printf ("%s (%s) %s\n", bus, busname, device);
  free (busname);
  return 0;
}

int access_driver (const char * driver) {
  struct stat statbuf;
  DIR * eeproms;
  struct dirent * eeprom;
  char eeprom_filename[256];
  int count = 0;

  snprintf (eeprom_filename, sizeof (eeprom_filename),
            "/sys/bus/i2c/drivers/%s", driver);

  if (!(eeproms = opendir (eeprom_filename)))
    return -1;

  while ((eeprom = readdir (eeproms))) {
    if (eeprom->d_name[0] == '.')
      continue;
    snprintf (eeprom_filename, sizeof (eeprom_filename),
	      "/sys/bus/i2c/drivers/%s/%s/eeprom", driver, eeprom->d_name);
    if (!(stat (eeprom_filename, &statbuf)))
      if (!do_eeprom_from_file (eeprom_filename))
        count++;
  }

  return count;
}

typedef int (*adapter_func) (const char *);

int try_at24 (const char * adapter) {
  struct stat statbuf;
  char filename[256];
  int count = 0, mask = 0, i;
  char * id = strchr (adapter, '-') + 1;
  FILE * file;

  snprintf (filename, sizeof (filename),
	    "/sys/class/i2c-adapter/%s/new_device", adapter);
  file = fopen (filename, "w");
  if (!file)
    return 0;
  for (i=0; i<8; i++) {
    snprintf (filename, sizeof (filename),
	      "/sys/class/i2c-adapter/%s/%s-%04x", adapter, id, i+0x50);
    if (stat (filename, &statbuf) == -1 && errno == ENOENT) {
      mask |= 1 << i;
      fprintf (file, "spd 0x%x\n", i+0x50);
      fflush (file);
    }
  }
  fclose (file);

  if (mask) {
    count = access_driver ("at24");
    snprintf (filename, sizeof (filename),
	      "/sys/class/i2c-adapter/%s/delete_device", adapter);
    file = fopen (filename, "w");
    if (file) {
      for (i=0; i<8; i++)
        if (mask & (1 << i)) {
          fprintf (file, "0x%x\n", i+0x50);
          fflush (file);
        }
      fclose (file);
    }
  }

  return count;
}

int foreach_i2c_adapter (adapter_func callback, int all) {
  struct stat statbuf;
  int result;
  DIR * sysfsdir;
  struct dirent * i2cadapter;
  int count = 0;
  char * name;

  if ((result = stat ("/sys/class/i2c-adapter", &statbuf))) {
    fprintf (stderr, "Can't stat() /sys/class/i2c-adapter: %s\n", strerror (errno));
    return -1;
  }
  if (!S_ISDIR(statbuf.st_mode)) {
    fprintf (stderr, "/sys/class/i2c-adapter is not a directory.\n");
    return -1;
  }

  if (!(sysfsdir = opendir ("/sys/class/i2c-adapter"))) {
    fprintf (stderr, "Can't opendir /sys/class/i2c-adapter: %s\n", strerror (errno));
    return -1;
  }

  while ((i2cadapter = readdir (sysfsdir)))
    if (i2cadapter->d_name[0] != '.') {
      name = get_i2c_bus_name (strchr (i2cadapter->d_name, '-')+1);
      if ((strncasecmp (name, "smbus", 5) && all == 1) ||
	  (!strncasecmp (name, "smbus", 5) && all == 0))
        count += callback (i2cadapter->d_name);
      free (name);
    }
  closedir (sysfsdir);
  return count;
}

int main () {
  int eeprom_detected = 0;
  int at24_detected = 0;

  switch (access_driver ("eeprom")) {
  case  0: eeprom_detected = 1; // Fall Through
  case -1: break;
  default: return 0;
  }
  switch (access_driver ("at24")) {
  case  0: at24_detected = 1; // Fall Through
  case -1: break;
  default: return 0;
  }

  if (at24_detected) {
    if (foreach_i2c_adapter (try_at24, 0) > 0)
      return 0;
    if (foreach_i2c_adapter (try_at24, 1) > 0)
      return 0;
  }


  return 0;
}
