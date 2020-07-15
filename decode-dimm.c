#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/errno.h>
#include <dirent.h>
#include <math.h>
#include <fcntl.h>

#include "vendors.h"
#include "constants.h"
#include "struct.h"
#include "i2c-tools-i2c-dev.h"
#include "sdr-ddr2.h"
#include "ddr3.h"
#include "ddr4.h"

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

int get_eeprom_memreq (const unsigned char *eeprom, int length) {
    switch (eeprom[2]) {
    case MEMTYPE_SDR:
    case MEMTYPE_DDR:
    case MEMTYPE_DDR2:
    case MEMTYPE_DDR3:
        return 256;
    case MEMTYPE_DDR4:
    case MEMTYPE_DDR4E:
        return get_ddr4_memreq ((struct ddr4_sdram_spd *) eeprom, length);
    case 0xff:
        if (eeprom[0] == 0x00 && eeprom[1] == 0xff && eeprom[2] == 0xff
            && eeprom[3] == 0xff && eeprom[4] == 0xff && eeprom[5] == 0xff
            && eeprom[6] == 0xff && eeprom[7] == 0x00)
            return 128;
    default:
        return -1;
    }
    return 0;
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
    case MEMTYPE_DDR4:
    case MEMTYPE_DDR4E:
        do_ddr4 ((struct ddr4_sdram_spd *) eeprom, length);
        break;
    case 0xff:
        if (eeprom[0] == 0x00 && eeprom[1] == 0xff && eeprom[2] == 0xff &&
            eeprom[3] == 0xff && eeprom[4] == 0xff && eeprom[5] == 0xff &&
            eeprom[6] == 0xff && eeprom[7] == 0x00)
            return -2;
    default:
        printf ("Unsupported memory type %d\n", eeprom[2]);
        return -1;
    }
    return 0;
}


int read_data_ioctl (int device, int features, unsigned char * buffer) {
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
        if (result < 0) {
            printf ("result: %s %d\n", strerror (errno), errno);
            break;
        }
        buffer[address] = result & 0xff;
        if (has_word)
            buffer[address + 1] = (result >> 8) & 0xff;
        bytes_read += increment;
    }

    return bytes_read;
}

int read_data (int device, int features, unsigned char * buffer) {
    int result;
    unsigned char address = 0;

    result = write (device, &address, 1);
    if (result < 0) {
        if (errno == EOPNOTSUPP)
            return read_data_ioctl (device, features, buffer);
        fprintf (stderr, "Failed to reset address: %d %s\n", errno, strerror (errno));
        return 0;
    }
    result = read (device, buffer, 256);
    if (result < 0) {
        if (errno == EOPNOTSUPP)
            return read_data_ioctl (device, features, buffer);
        fprintf (stderr, "Failed to read from device: %s\n", strerror (errno));
        return 0;
    }
    return result;
}

int set_ee1004_bank (int device, int bank, int client) {
    union i2c_smbus_data data;
    int result;
    int SPA = bank ? EE1004_SPA1 : EE1004_SPA0;

    if (ioctl (device, I2C_SLAVE_FORCE, SPA)) {
        fprintf (stderr, "Can't select client 0x%02x: %s\n", SPA, strerror (errno));
        return -1;
    }

    data.byte = 0;
    result = i2c_smbus_access (device, I2C_SMBUS_WRITE, 0, I2C_SMBUS_BYTE, &data);
    if (!result && client >= 0) {
        result = ioctl (device, I2C_SLAVE_FORCE, client);
        if (result) {
            fprintf (stderr, "Can't select client 0x%02x: %s\n", client, strerror (errno));
        }
    }

    return result;
}

int scan_adapter (const char *adapter) {
    char dev_name[512];
    struct stat statbuf;
    int result, features;
    int device, client;
    unsigned char eeprom[512];
    int count = 0;
    int required;
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
        fprintf (stderr, "Doesn't support byte or word read\n");
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
            required = get_eeprom_memreq (eeprom, bytes_read);
            if (bytes_read == 256 && required > 256) {
                set_ee1004_bank (device, 1, client);
                result = read_data (device, features, eeprom + 256);
                set_ee1004_bank (device, 0, client);
                if (result == 256) {
                    if (memcmp (eeprom, eeprom + 256, 256))
                        bytes_read += 256;
                }
            }
            result = do_eeprom (client, eeprom, bytes_read);
            if (result == 0)
                count++;
        } else printf ("no data from client 0x%02x\n", client);
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
    foreach_i2c_adapter (1);

    return 0;
}
