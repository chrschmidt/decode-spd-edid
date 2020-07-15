#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <string.h>
#include <math.h>

#include "eedid_struct.h"
#include "eedid_constants.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))
#endif

int version = 0, ctf = 0;

static char * get_eedid_string (const char * in) {
    static char string[14];
    int i;
    memcpy (string, in, 13);
    string[14] = 0;
    for (i=0; i<13; i++)
        if (string[i] == 0x0a) {
            string[i] = 0;
            break;
        }

    return string;
}

static void print_dtd (const struct detailed_timing_t * dtd) {
    int width, height;
    int hsyncstart, hsyncend, htotal;
    int vsyncstart, vsyncend, vtotal;
    float hclock, vclock;

    width = dtd->horz_act_lo +
        (((int)(dtd->horz_act_blank_hi >> 4) & 15) << 8);
    hsyncstart = width + dtd->horz_sync_offset +
        (((int)(dtd->sync_hi >> 6) & 3) << 8);
    hsyncend = hsyncstart + dtd->horz_sync_width +
        (((int)(dtd->sync_hi >> 4) & 3) << 8);
    htotal = width + dtd->horz_blank_lo +
        ((int)(dtd->horz_act_blank_hi & 15) << 8);
    hclock = (float)dtd->pixel_clock * 10 / (float)htotal;
    height = dtd->vert_act_lo +
        (((int)(dtd->vert_act_blank_hi >> 4) & 15) << 8);
    vsyncstart = height + ((dtd->vert_sync_offset_width >> 4) & 15) +
        (((int)(dtd->sync_hi >> 2) & 3) << 8);
    vsyncend = vsyncstart + (dtd->vert_sync_offset_width & 15) +
        ((int)(dtd->sync_hi & 3) << 8);
    vtotal = height + dtd->vert_blank_lo +
        ((int)(dtd->vert_act_blank_hi & 15) << 8);
    vclock = round (hclock * 1000.0 / (float)vtotal);
    printf ("Detailed Timing: %dx%d@%d (%6.2f %d %d %d %d %d %d %d %d%s",
            width, height, (int)vclock,
            (float)le16toh (dtd->pixel_clock) / 100.0,
            width,  hsyncstart, hsyncend, htotal,
            height, vsyncstart, vsyncend, vtotal,
            dtd->flags & 0x80 ? " Interlace" : "");
    switch (dtd->flags & 24) {
    case 0:
    case 8:
        printf (" Composite%s)\n", dtd->flags & 0x02 ? "" : " SyncOnGreen");
        break;
    case 16:
        printf (" Composite %cCSync)\n", dtd->flags & 0x02 ? '+' : '-');
        break;
    case 24:
        printf (" %cHSync %cVSync)\n",
                dtd->flags & 0x02 ? '+' : '-',
                dtd->flags & 0x04 ? '+' : '-');
        break;
    }
    switch (((dtd->flags >> 4) & 6) | (dtd->flags & 1)) {
    case 0:
    case 1: break;
    case 2: printf ("  Field sequential stereo, right image on sync\n"); break;
    case 3: printf ("  2-way interleaved stereo, right image on even lines\n"); break;
    case 4: printf ("  Field sequential stereo, left image on sync\n"); break;
    case 5: printf ("  2-way interleaved stereo, left image on even lines\n"); break;
    case 6: printf ("  4-way interleaved stereo\n"); break;
    case 7: printf ("  Side-by-Side interleaved stereo\n");
    }
}

static void print_18b (const union eighteen_bytes_descriptor_t * desc) {
    if (desc->timing.pixel_clock)
        print_dtd ((struct detailed_timing_t *)desc);
    else if (desc->desc.flag2 != 0 || (desc->desc.flag3!= 0 && version < v14))
        printf ("Warning: invalid device descriptor block\n");
    else switch (desc->desc.tag) {
        case dt_dummy:
            break;
        case dt_serial:
            printf ("Serial number: %s\n",
                    get_eedid_string ((char *)desc->desc.data));
            break;
        case dt_rangelimits:
            printf ("Range limits: Max clock = %dMHz, Refresh = %d-%dHz, HSync %d-%dkHz",
                    desc->limits.max_pixel_clock * 10,
                    (int)desc->limits.min_vert_rate + ((desc->limits.offsets & 0x01) ? 255:0),
                    (int)desc->limits.max_vert_rate + ((desc->limits.offsets & 0x02) ? 255:0),
                    (int)desc->limits.min_horz_rate + ((desc->limits.offsets & 0x04) ? 255:0),
                    (int)desc->limits.max_horz_rate + ((desc->limits.offsets & 0x08) ? 255:0));
            if (ctf)
                switch (desc->limits.video_timing_support) {
                case 0:
                case 1: break;
                case 2: printf (", Secondary GTF informationen\n"); break;
                case 4: printf (", CVT information\n"); break;
                default:
                    printf ("Invalid video timing support (%02x)\n",
                            desc->limits.video_timing_support);
                    break;
                }
            printf ("\n");
            break;
        case dt_string:
            printf ("String: %s\n", get_eedid_string ((char *)desc->desc.data));
            break;
        case dt_name:
            printf ("Name: %s\n", get_eedid_string ((char *)desc->desc.data));
            break;
        default:
            printf ("Unhandled tag 0x%02x\n", desc->desc.tag);
        }
}

static void print_base_eedid (const struct eedid_t * eedid) {
    const uint8_t eedid_header[8] =
        { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };

    int i;
    char vendor[4];
    uint16_t manufacturer;
    int is_digital = (eedid->video_input_definition & 128) == 128;
    int csum = 0;

    for (i=0; i<128; i++)
        csum += ((uint8_t *)eedid)[i];
    csum &= 255;

    for (i=0; i<8; i++)
        if (eedid_header[i] != eedid->header[i])
            return;

    version = 1000*eedid->edid_version + eedid->edid_revision;
    printf ("EDID Version %d.%d, checksum %sok\n",
            eedid->edid_version,
            eedid->edid_revision,
            csum ? "not ": "");

    manufacturer = be16toh (eedid->id_manufacturer_name);
    vendor[0] = ((manufacturer >> 10) & 31) + '@';
    vendor[1] = ((manufacturer >> 5) & 31) + '@';
    vendor[2] = (manufacturer & 31) + '@';
    vendor[3] = 0;
/* XXX Check for extended info from extra blocks in 1.1+ */
    printf ("Vendor: %s Product: %d",
            vendor,
            le16toh (eedid->id_product_code));
    switch (eedid->id_serial_number) {
    case 0x00000000:
    case 0x01010101:
        break;
    default:
        printf (" Serial number: %d (%x)", le32toh (eedid->id_serial_number),
                eedid->id_serial_number);
    }
    printf ("\n");
    if (eedid->year_of_manufacture) {
        switch (eedid->week_of_manufacture) {
        case 0x00:
            printf ("Manufacture year: %d\n",
                    1990+eedid->year_of_manufacture);
            break;
        case 0xff:
            printf ("Model Year: %d\n", 1990+eedid->year_of_manufacture);
            break;
        default:
            if (eedid->week_of_manufacture < 0x37)
                printf ("Manufacture date: Week %d/%d\n",
                        eedid->week_of_manufacture,
                        1990+eedid->year_of_manufacture);
        }
    }

    if (!is_digital) {
        printf ("Analog Interface, voltage level %s, ",
                voltagelevelstrings [(eedid->video_input_definition >> 5) & 3]);
        if (eedid->video_input_definition & 16)
            printf ("blank-to-black setup/pedestal\n");
        else printf ("blank level = black level\n");
        if (eedid->video_input_definition & 8)
            printf ("  Separate H&V Sync supported\n");
        if (eedid->video_input_definition & 4)
            printf ("  Composite Sync on HSync supported\n");
        if (eedid->video_input_definition & 2)
            printf ("  Sync on Green supported\n");
        if (eedid->video_input_definition & 1)
            printf ("  Serration on VSync supported");
    } else {
        printf ("Digital Interface");
        switch (version) {
        case v13:
            if (eedid->video_input_definition & 1)
                printf (", compatible with VESA DFP 1.x TMDS");
            break;
        case v14:
            if ((eedid->video_input_definition & 15) &&
                ((eedid->video_input_definition & 15) < 15))
                printf (", type %s",
                        ifnames[eedid->video_input_definition & 15]);
            if (((eedid->video_input_definition >> 4) & 7) &&
                (((eedid->video_input_definition >> 4) & 7) < 7))
                printf (", %d bits per primary color",
                        bppvalues[(eedid->video_input_definition >> 4) & 7]);
            break;
        }
        printf ("\n");
    }

    if (eedid->horz_size_ar && eedid->vert_size_ar) {
        printf ("Screen size: %dx%dcm\n",
                eedid->horz_size_ar, eedid->vert_size_ar);
    } else if (version >= v14 && eedid->horz_size_ar) {
        printf ("Landscape display, aspect ratio ");
        switch (eedid->horz_size_ar) {
            /* Stored Value = (Aspect Ratio * 100) - 99 */
        case 26: printf ("5:4\n"); break;
        case 34: printf ("4:3\n"); break;
        case 61: printf ("16:10\n"); break;
        case 79: printf ("16:9\n"); break;
        default: printf ("%4.2f:1\n",
                         ((float)eedid->horz_size_ar + 99.0) / 100.0);
        }
    } else if (version >= v14 && eedid->vert_size_ar) {
        printf ("Portrait display, aspect ratio ");
        switch (eedid->horz_size_ar) {
            /* Stored Value = (100 / Aspect Ratio) - 99 */
        case 26: printf ("5:4\n"); break;
        case 34: printf ("3:4\n"); break;
        case 61: printf ("10:16\n"); break;
        case 79: printf ("9:16\n"); break;
        default: printf ("%4.2f:1\n",
                         100 / ((float)eedid->horz_size_ar + 99.0));
        }
    }

    /* XXX Gamma = 0xff means take from extended block */
    if (eedid->gamma)
        printf ("Gamma: %4.2f\n", ((float)eedid->gamma + 100.0) / 100.0);

    switch ((eedid->features >> 5) & 7) {
    case 0: printf ("No power mananagement support.\n"); break;
    case 1: printf ("DPM compliant power management.\n"); break;
    default:
        printf ("DPMS compliant power management, supported modes:");
        if (eedid->features & 128) printf (" Standby");
        if (eedid->features & 64) printf (" Suspend");
        if (eedid->features & 32) printf (" Active-Off");
        printf ("\n");
    }
    switch (version) {
    case v14:
        if (is_digital)
            printf ("Color Encoding: %s\n",
                    colorformatnames[(eedid->features >> 3) & 3]);
        else
            printf ("Color Type: %s\n",
                    colortypenames[(eedid->features >> 3) & 3]);
        printf ("sRGB is%s the default color space.\n",
                (eedid->features & 4) ? "" : " not");
        printf ("Preferred timing does%s include the native pixel format and refresh rate.\n",
                (eedid->features & 2) ? "" : " not");
        printf ("Display is of %scontinuous frequency type.\n",
                (eedid->features & 1) ? "" : " non-");
        break;
    default:
        printf ("Color Type: %s\n",
                colortypenames[(eedid->features >> 3) & 3]);
        printf ("sRGB is%s the default color space.\n",
                (eedid->features & 4) ? "" : " not");
        if (eedid->features & 2)
            printf ("First detailed timing is preferred timing.\n");
        if (eedid->features & 1)
            printf ("Display supports timings based on default GTF standard values.\n");
    }
    ctf = eedid->features & 1;

    for (i=0; i<16; i++)
        if (be16toh (eedid->established_timings) & (1 << i))
            printf ("Supported Established Timing: %s\n",
                    establishedtimingnames[i]);
    for (i=0; i<8; i++)
        if (be16toh (eedid->manufacturer_timings) & (1 << i))
            printf ("Supported Manufacturer's Timing: %s\n",
                    manufacturertimingnames[i]);
    for (i=0; i<8; i++) {
        int x, y, r;
        switch (eedid->standard_timings[i]) {
        case 0x0000: /* Reserved */
        case 0x0101: /* Unused */
        case 0x2020: /* Workaround for known broken old devices */
            break;
        default:
            x = ((int) (le16toh (eedid->standard_timings[i]) & 255) + 31) << 3;
            switch ((be16toh (eedid->standard_timings[i]) >> 6) & 3) {
            case 0:
                if (version < v13) y = x;
                else y = 10*x / 16;
                break;
            case 1:
                y = 3*x / 4;
                break;
            case 2:
                y = 4*x / 5;
                break;
            case 3:
                y = 9*x / 16;
                break;
            }
            /* No rule without exception - HDTV mode is not encodable */
            if ((x == 1360 && y == 765) || (x == 1368 && y == 769)) {
                x = 1366;
                y = 768;
            }
            r = (be16toh (eedid->standard_timings[i]) & 63) + 60;
            printf ("Supported Standard Timing: %dx%d@%d\n", x, y, r);
        }
    }

    for (i=0; i<4; i++)
        print_18b (&eedid->detailed_timings[i]);
}

static void handle_extended_cea (const unsigned char * data) {
    int type = data[1];
    int length = (data [0] & 31) - 1;

    switch (type) {
    case cea_colorimetry:
        break;
    default:
        printf ("Unhandled CEA Extended Data Block id %d length %d\n",
                type, length);
    }
}

static void handle_cea_audio (const unsigned char * data, int length) {
    int i = 0;

    printf ("CEA Audio Data Block\n");
    while (i < length-2) {
        int format = (data[i] >> 3) & 15;
        if (format == 0xf) format = ((data[i+2] >> 3) & 31) + 15;
        printf ("  Codec %s, max %d channels",
                cea861_audio_format_name[format],
                (data[i] & 7) + 1);
        if (data[i+1] & 0x01) printf (", 32kHz");
        if (data[i+1] & 0x02) printf (", 44.1kHz");
        if (data[i+1] & 0x04) printf (", 48kHz");
        if (data[i+1] & 0x08) printf (", 88.2kHz");
        if (data[i+1] & 0x10) printf (", 96kHz");
        if (data[i+1] & 0x20) printf (", 176.4kHz");
        if (data[i+1] & 0x40) printf (", 192kHz");

        if (format == cea_audio_lpcm) {
            if (data[i+2] & 0x01) printf (", 16bit");
            if (data[i+2] & 0x02) printf (", 20bit");
            if (data[i+2] & 0x04) printf (", 24bit");
        } else if (format >= cea_audio_ac3 && format <= cea_audio_atrac) {
            printf (", maximum bitrate %dkbit", 8*data[i+2]);
        } else if (format == cea_audio_wmapro)
            printf (", Profile %d", data[i+2] & 7);
        printf ("\n");
        i+=3;
    }
}

static void handle_cea_video (const unsigned char * data, int length) {
    int i = 0;

    printf ("CEA Video Data Block\n");
    for (i=0; i<length; i++) {
        unsigned int mode = (data[i] & 127) - 1;
        printf ("  CEA Timing %s%s\n",
                (mode < 64) ? cea_mode_names [mode] : "(unknown)",
                data[i] & 128 ? " (native)" : "");
    }
}

static void handle_cea_vendor_hdmi (const unsigned char * data, int length) {
    struct cea861_vendor_hdmi * hdmi = (struct cea861_vendor_hdmi *) data;
    uint16_t address = le16toh (hdmi->phys_address);
    int printed = 0;

    printf ("HDMI Vendor Specific Data Block\n");
    printf ("  HDMI address %0x.%0x.%0x.%0x \n",
            (address >> 4) & 15, address & 15,
            (address >> 12) & 15, (address >> 8) & 15);
    if (hdmi->video_flags) {
        printf ("  Supports ");
#define lprint(x) printf ("%s%s", printed++ ? ", ": "", x)
        if (hdmi->video_flags & 0x80) lprint ("ACP/ISRC1/ISRC2 packets");
        if (hdmi->video_flags & 0x40) lprint ("48bpp");
        if (hdmi->video_flags & 0x20) lprint ("36bpp");
        if (hdmi->video_flags & 0x10) lprint ("30bpp");
        if (hdmi->video_flags & 0x08) lprint ("YCbCr in Deep Color Modes");
        if (hdmi->video_flags & 0x01) lprint ("Dual-Link DVI");
#undef lprint
        printf ("\n");
    }
    if (hdmi->max_tmds_clock)
        printf ("  Maximum TMDS clock: %dMHz\n", 5*hdmi->max_tmds_clock);
    switch (hdmi->latency_fields & 0xc0) {
    case 0x00:
        break;
    case 0x40:
        printf ("  Invalid latency flag.\n");
        break;
    case 0x80:
        printf ("  Latency: ");
        if (hdmi->video_latency && hdmi->video_latency<255)
            printf ("Video: %dms", (hdmi->video_latency-1) * 2);
        if (hdmi->audio_latency && hdmi->audio_latency<255)
            printf ("%sAudio: %dms",
                    (hdmi->video_latency && hdmi->video_latency<255)?" ":"",
                    (hdmi->audio_latency-1) * 2);
        printf ("\n");
        break;
    case 0xc0:
        printf ("  Latency for progressive operation: ");
        if (hdmi->video_latency && hdmi->video_latency<255)
            printf ("Video: %dms", (hdmi->video_latency-1) * 2);
        if (hdmi->audio_latency && hdmi->audio_latency<255)
            printf ("%sAudio: %dms",
                    (hdmi->video_latency && hdmi->video_latency<255)?" ":"",
                    (hdmi->audio_latency-1) * 2);
        printf ("\n");
        printf ("  Latency for interlaced operation: ");
        if (hdmi->interlaced_video_latency && hdmi->interlaced_video_latency<255)
            printf ("Video: %dms", (hdmi->interlaced_video_latency-1) * 2);
        if (hdmi->interlaced_audio_latency && hdmi->interlaced_audio_latency<255)
            printf ("%sAudio: %dms",
                    (hdmi->interlaced_video_latency &&
                     hdmi->interlaced_video_latency<255)?" ":"",
                    (hdmi->interlaced_audio_latency-1) * 2);
        printf ("\n");
        break;
    }
}

void handle_cea_vendor (const unsigned char * data, int length) {
    struct cea861_vendor * vendor = (struct cea861_vendor *) data;

    if (length < 3)
        return;

    if (vendor->id_lo == 0x03 && vendor->id_mid == 0x0c && vendor->id_hi == 0x00)
        handle_cea_vendor_hdmi (data+3, length-3);
    else
        printf ("Unknown Vendor %02x%02x%02x Specific Data Block\n",
                vendor->id_hi, vendor->id_mid, vendor->id_lo);
}

void handle_cea_speaker (const unsigned char * data, int length) {
    int printed = 0;

    if (length != 3)
        return;

    printf ("CEA Speaker Data Block\n");
#define lprint(x) printf ("%s%s", printed++ ? ", ": "  ", x)
    if (data[0] & 0x01) lprint ("Front Right/Left");
    if (data[0] & 0x02) lprint ("LFE");
    if (data[0] & 0x04) lprint ("Front Center");
    if (data[0] & 0x08) lprint ("Rear Right/Left");
    if (data[0] & 0x10) lprint ("Rear Center");
    if (data[0] & 0x20) lprint ("Front Right/Left Center");
    if (data[0] & 0x40) lprint ("Rear Right/Left Center");
    if (data[0] & 0x80) lprint ("Front Right/Left Wide");
    if (data[1] & 0x01) lprint ("Front Right/Left High");
    if (data[1] & 0x02) lprint ("Top Center");
    if (data[1] & 0x04) lprint ("Front Center High");
#undef lprint
    printf ("\n");
}

static void handle_cea (const unsigned char * data) {
    int type = (data[0] >> 5) & 7;
    int length = data[0] & 31;

    switch (type) {
    case cea_audio:
        handle_cea_audio (data+1, length);
        break;
    case cea_video:
        handle_cea_video (data+1, length);
        break;
    case cea_vendor:
        handle_cea_vendor (data+1, length);
        break;
    case cea_speaker:
        handle_cea_speaker (data+1, length);
        break;
    case cea_extended:
        handle_extended_cea (data);
        break;
    default:
        printf ("Unhandled CEA Data block id %d length %d\n", type, length);
    }
}

static void print_cea861 (const struct eedid_ext_cea861 * ext) {
    int i, csum = 0;
    int n = (127 - ext->_18b_start) / 18;

    for (i=0; i<128; i++)
        csum += ((uint8_t *)ext)[i];
    csum &= 255;

    printf ("CEA-681 Data Structure Version %d, checksum %sok\n",
            ext->version,
            csum?"not ":"");
    if (csum)
        return;

    if (ext->flags & 240) {
        int printed = 0;
        printf ("Device ");
        if (ext->flags & 0x80) {
            printf ("underscans IT video formats");
            printed = 1;
        }
        if (ext->flags & 0x70)
            printf ("%ssupports ", printed ? ", " : "");
        if (ext->flags & 0x40) {
            printf ("audio");
            printed = 2;
        }
        if (ext->flags & 0x30) {
            printf ("%sYCbCr 4:4:4 / 4:2:2", printed == 2? ", " : "");
            if ((ext->flags & 0x30) != 0x30)
                printf ("\nWarning: invalid YCbCr support (0x02%x != 0x30)",
                        ext->flags & 0x30);
        }
        printf ("\n");
    }

    for (i=0; i<n; i++)
        print_18b ((union eighteen_bytes_descriptor_t *)&ext->data[ext->_18b_start - 4 + 18*i]);

    if (ext->version >= 3) {
        i=0;
        while (i<(ext->_18b_start - 4)) {
            handle_cea (&ext->data[i]);
            i += 1 + (ext->data[i] & 31);
        }
    }
}

static void handle_extension (const unsigned char * data) {
    switch (data[0]) {
    case ext_cea861:
        print_cea861 ((struct eedid_ext_cea861 *)data);
        break;
    default:
        printf ("Unhandled extension %02x\n", data[0]);
    }
}

int get_eedid_memreq (const struct eedid_t * eeprom, int length) {
    int needed_length;
    needed_length = 128*(eeprom->extension_block_count+1);

    if (needed_length > 256) {
      printf ("Warning: eeprom reader >256 needed");
      return 256;
    }
    return needed_length;
}

void do_eedid (const struct eedid_t * eeprom, int length) {
    print_base_eedid (eeprom);

    if (length < 128*(eeprom->extension_block_count+1)) {
        printf ("Warning: %d bytes expected, only %d bytes passed!\n",
                128*(eeprom->extension_block_count+1), length);
        return;
    }
    if (eeprom->extension_block_count == 1) {
      handle_extension ((unsigned char *)eeprom+128);
    } else if (eeprom->extension_block_count > 1) {
        printf ("More than one Extension (%d found) not supported.\n",
                eeprom->extension_block_count);
    }
}
