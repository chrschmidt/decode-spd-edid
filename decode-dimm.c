#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <fcntl.h>

#include "vendors.h"
#include "constants.h"
#include "struct.h"
#include "i2c-tools-i2c-dev.h"
#include "sdr-ddr2.h"
#include "ddr3.h"

char *get_i2c_bus_name (const char *id) {
    char bus[256];
    char busname[256];
    struct stat statbuf;
    FILE *file;

    snprintf (bus, sizeof (bus), "/sys/class/i2c-dev/i2c-%s/device/name", id);
    if (stat (bus, &statbuf)) {
        snprintf (bus, sizeof (bus), "/sys/class/i2c-dev/i2c-%s/name", id);
        if (stat (bus, &statbuf))
            return NULL;
    }

    busname[0] = 0;
    if (!(file = fopen (bus, "r")))
        return NULL;
    fgets (busname, sizeof (busname), file);
    fclose (file);
    busname[strlen (busname) - 1] = 0;

    return strdup (busname);
}

int do_eeprom (int device, const unsigned char *eeprom, int length) {
    printf ("Analyzing client 0x%02x\n", device);
    switch (eeprom[2]) {
    case MEMTYPE_SDR:
    case MEMTYPE_DDR:
    case MEMTYPE_DDR2:
        do_sdram ((struct sdram_spd *) eeprom, length);
        break;
    case MEMTYPE_DDR3:
        do_ddr3 ((struct ddr3_sdram_spd *) eeprom, length);
        break;
    case 0xff:
        if (eeprom[0] == 0x00 && eeprom[1] == 0xff && eeprom[2] == 0xff
            && eeprom[3] == 0xff && eeprom[4] == 0xff && eeprom[5] == 0xff
            && eeprom[6] == 0xff && eeprom[7] == 0x00)
            return -2;
    default:
        printf ("Unsupported memory type %d\n", eeprom[2]);
        return -1;
    }
    return 0;
}

int read_data (int device, int features, unsigned char * buffer) {
    int has_word = features & I2C_FUNC_SMBUS_READ_WORD_DATA;
    int address, retry, result;
    int bytes_read = 0;
    int increment = has_word ? 2 : 1;

    for (address = 0; address < 256; address += increment) {
        retry = 0;
        do {
            retry++;
            if (has_word)
                result = i2c_smbus_read_word_data (device, address);
            else
                result = i2c_smbus_read_byte_data (device, address);
        } while (result < 0 && retry < 5);
        if (result < 0)
            break;
        buffer[address] = result & 0xff;
        if (has_word)
            buffer[address + 1] = (result >> 8) & 0xff;
        bytes_read += increment;
    }

    return bytes_read;
}

int scan_adapter (const char *adapter) {
    char dev_name[512];
    struct stat statbuf;
    int result, features;
    int device, client;
    unsigned char eeprom[512];
    int count = 0;
    int bytes_read;

    snprintf (dev_name, 256, "/dev/%s", adapter);
    if ((result = stat (dev_name, &statbuf))) {
        fprintf (stderr, "Can't stat() %s: %s\n", dev_name, strerror (errno));
        return -1;
    }
    if (!S_ISCHR (statbuf.st_mode)) {
        fprintf (stderr, "%s is not a character device.\n", dev_name);
        return -1;
    }

    device = open (dev_name, O_RDWR);
    if (device < 0) {
        fprintf (stderr, "Can't open %s: %s\n", dev_name, strerror (errno));
        return -1;
    }

    if (ioctl (device, I2C_FUNCS, &features)) {
        fprintf (stderr, "Can't retrieve features: %s\n", strerror (errno));
        return -1;
    }

    if (!(features & (I2C_FUNC_SMBUS_READ_BYTE_DATA | I2C_FUNC_SMBUS_READ_WORD_DATA))) {
        return -1;
    }

    if ((features & I2C_FUNC_SMBUS_PEC) && ioctl (device, I2C_PEC, 0)) {
        fprintf (stderr, "Can't disable PEC: %s\n", strerror (errno));
        return -1;
    }

    if ((features & I2C_FUNC_10BIT_ADDR) && ioctl (device, I2C_TENBIT, 0)) {
        fprintf (stderr, "Can't disable tenbit: %s\n", strerror (errno));
        return -1;
    }

    for (client = 0x50; client < 0x58; client++) {
        if (ioctl (device, I2C_SLAVE_FORCE, client)) {
            fprintf (stderr, "Can't select client 0x%02x: %s\n", client,
                     strerror (errno));
            continue;
        }
        bytes_read = read_data (device, features, eeprom);
        if (bytes_read > 0) {
            result = do_eeprom (client, eeprom, bytes_read);
            if (result == 0)
                count++;
        }
    }
    close (device);
    return count;
}

int foreach_i2c_adapter (int all) {
    struct stat statbuf;
    int result;
    DIR *sysfsdir;
    struct dirent *i2cadapter;
    int count = 0;
    char *name;

    if ((result = stat ("/sys/class/i2c-dev", &statbuf))) {
        fprintf (stderr, "Can't stat() /sys/class/i2c-dev: %s\n",
                 strerror (errno));
        return -1;
    }
    if (!S_ISDIR (statbuf.st_mode)) {
        fprintf (stderr, "/sys/class/i2c-dev is not a directory.\n");
        return -1;
    }

    if (!(sysfsdir = opendir ("/sys/class/i2c-dev"))) {
        fprintf (stderr, "Can't opendir /sys/class/i2c-dev: %s\n",
                 strerror (errno));
        return -1;
    }

    while ((i2cadapter = readdir (sysfsdir)))
        if (i2cadapter->d_name[0] != '.') {
            name = get_i2c_bus_name (strchr (i2cadapter->d_name, '-') + 1);
            if (all == 1 || (!strncasecmp (name, "smbus", 5) && all == 0)) {
                printf ("Testing %s (%s)\n", i2cadapter->d_name, name);
                count += scan_adapter (i2cadapter->d_name);
            }

            free (name);
        }
    closedir (sysfsdir);
    return count;
}

int main () {
    if (foreach_i2c_adapter (0) > 0)
        return 0;
    if (foreach_i2c_adapter (1) > 0)
        return 0;

    return 0;
}
