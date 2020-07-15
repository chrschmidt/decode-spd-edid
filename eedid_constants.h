#pragma once

enum eedid_versions {
    v10 = 1000,
    v11 = 1001,
    v12 = 1002,
    v13 = 1003,
    v14 = 1004 };

enum bitdepths {
    bpp_undefined = 0,
    bpp_6,
    bpp_8,
    bpp_10,
    bpp_12,
    bpp_14,
    bpp_16,
    bpp_reserved };

static const int bppvalues[8] = { 0, 6, 8, 10, 12, 14, 16, -1 };

enum iftypes {
    if_undefined = 0,
    if_dvi,
    if_hdmi_a,
    if_hdmi_b,
    if_mddi,
    if_displayport };

static const char * ifnames[16] = {
    "undefined", "DVI", "HDMI-a", "HDMI-b", "MDDI", "DisplayPort",
    "reserved", "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved", "reserved" };

enum voltagelevels {
    vl_07_03 = 0,
    vl_0714_0286,
    vl_1_04,
    vl_07_0 };

static const char * voltagelevelstrings[4] = {
    "0.700 : 0.300 : 1.000 Vp-p",
    "0.714 : 0.286 : 1.000 Vp-p",
    "1.000 : 0.400 : 1.400 Vp-p",
    "0.700 : 0.000 : 0.700 Vp-p" };

enum colortypes {
    color_gray = 0,
    color_rgb,
    color_nonrgb,
    color_undefined };

static const char * colortypenames[4] = {
    "monochrome/grayscale",
    "RGB color",
    "non-RGB color",
    "undefined" };

enum colorformats {
    rgb444 = 0,
    rgb444_ycrcb444,
    rgb444_ycrcb422,
    rgb444_ycrcb444_ycrcb422 };

static const char * colorformatnames[4] = {
    "RGB 4:4:4",
    "RGB 4:4:4 & YCrCb 4:4:4",
    "RGB 4:4:4 & YCrCb 4:2:2",
    "RGB 4:4:4 & YCrCb 4:4:4 & YCrCb 4:2:2" };

static const char * establishedtimingnames[16] = {
    "VESA 1280x1024@75",
    "VESA 1024x768@75",
    "VESA 1024x768@70",
    "VESA 1024x768@60",
    "IBM 1024x768@87 interlaced",
    "Apple Mac II 832x624@75",
    "VESA 800x600@75",
    "VESA 800x600@72",
    "VESA 800x600@60",
    "VESA 800x600@56",
    "VESA 640x480@75",
    "VESA 640x480@72",
    "Apple Mac II 640x480@67",
    "IBM VGA 640x480@60",
    "IBM XGA2 720x400@88",
    "IBM VGA 720x400@70" };

static const char * manufacturertimingnames[8] = {
    "Apple Mac II 1152x870@75",
    "undefined",
    "undefined",
    "undefined",
    "undefined",
    "undefined",
    "undefined",
    "undefined" };

enum descriptortypes {
    dt_vendor_0               = 0x00,
    dt_vendor_1               = 0x01,
    dt_vendor_2               = 0x02,
    dt_vendor_3               = 0x03,
    dt_vendor_4               = 0x04,
    dt_vendor_5               = 0x05,
    dt_vendor_6               = 0x06,
    dt_vendor_7               = 0x07,
    dt_vendor_8               = 0x08,
    dt_vendor_9               = 0x09,
    dt_vendor_a               = 0x0a,
    dt_vendor_b               = 0x0b,
    dt_vendor_c               = 0x0c,
    dt_vendor_d               = 0x0d,
    dt_vendor_e               = 0x0e,
    dt_vendor_f               = 0x0f,
    dt_dummy                  = 0x10,
    dt_DCM_data               = 0xf7,
    dt_CVT_timings            = 0xf8,
    dt_established_timings_3  = 0xf9,
    dt_extra_standard_timings = 0xfa,
    dt_colorpointdata         = 0xfb,
    dt_name                   = 0xfc,
    dt_rangelimits            = 0xfd,
    dt_string                 = 0xfe,
    dt_serial                 = 0xff };

enum extensions {
    ext_cea861                = 0x02,
    ext_edid_20               = 0x20,
    ext_di                    = 0x40,
    ext_ls                    = 0x50,
    ext_mi                    = 0x60,
    ext_blockmap              = 0xf0,
    ext_manufacturer          = 0xff };

static const char * cea861_audio_format_name[] = {
    "From Header", "LPCM", "AC-3", "MPEG-1", "MP3", "MPEG-2", "AAC LC", "DTS",
    "ATRAC", "DSD", "E-AC-3", "DTS-HD", "MLP", "DST", "WMA Pro",
    "(extended 15)", "HE-AAC", "HE-AACv2", "MPEG Surround" };

static const char * cea_mode_names[] = {
    "640x480p@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 1:1",
    "720x480p@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 8:9",
    "720x480p@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 32:27",
    "1280x720p@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080i@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 1:1",
    "720(1440)x480i@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 8:9",
    "720(1440)x480i@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 32:27",
    "720(1440)x240p@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 4:9",
    "720(1440)x240p@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 16:27",
    "2880x480i@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 2:9 - 20:92",
    "2880x480i@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 8:27 - 80:272",
    "2880x240p@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 1:9 - 10:92",
    "2880x240p@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 4:27 - 40:272",
    "1440x480p@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 4:9 or 8:93",
    "1440x480p@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 16:27 or 32:273",
    "1920x1080p@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 1:1",
    "720x576p@50Hz DAR: 4:3 PAR: 4:3 16:15",
    "720x576p@50Hz DAR: 16:9 PAR: 16:9 64:45",
    "1280x720p@50Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080i@50Hz DAR: 16:9 PAR: 16:9 1:1",
    "720(1440)x576i@50Hz DAR: 4:3 PAR: 4:3 16:15",
    "720(1440)x576i@50Hz DAR: 16:9 PAR: 16:9 64:45",
    "720(1440)x288p@50Hz DAR: 4:3 PAR: 4:3 8:15",
    "720(1440)x288p@50Hz DAR: 16:9 PAR: 16:9 32:45",
    "2880x576i@50Hz DAR: 4:3 PAR: 4:3 2:15 - 20:152",
    "2880x576i@50Hz DAR: 16:9 PAR: 16:9 16:45 - 160:452",
    "2880x288p@50Hz DAR: 4:3 PAR: 4:3 1:15 - 10:152",
    "2880x288p@50Hz DAR: 16:9 PAR: 16:9 8:45 - 80:452",
    "1440x576p@50Hz DAR: 4:3 PAR: 4:3 8:15 or 16:153",
    "1440x576p@50Hz DAR: 16:9 PAR: 16:9 32:45 or 64:453",
    "1920x1080p@50Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080p@23.97Hz/24Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080p@25Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080p@29.97Hz/30Hz DAR: 16:9 PAR: 16:9 1:1",
    "2880x480p@59.94Hz/60Hz DAR: 4:3 PAR: 4:3 2:9, 4:9, or 8:94",
    "2880x480p@59.94Hz/60Hz DAR: 16:9 PAR: 16:9 8:27, 16:27, or 32:274",
    "2880x576p@50Hz DAR: 4:3 PAR: 4:3 4:15, 8:15, or 16:154",
    "2880x576p@50Hz DAR: 16:9 PAR: 16:9 16:45, 32:45, or 64:454",
    "1920x1080i (1250 total)@50Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080i@100Hz DAR: 16:9 PAR: 16:9 1:1",
    "1280x720p@100Hz DAR: 16:9 PAR: 16:9 1:1",
    "720x576p@100Hz DAR: 4:3 PAR: 4:3 16:15",
    "720x576p@100Hz DAR: 16:9 PAR: 16:9 64:45",
    "720(1440)x576i@100Hz DAR: 4:3 PAR: 4:3 16:15",
    "720(1440)x576i@100Hz DAR: 16:9 PAR: 16:9 64:45",
    "1920x1080i@119.88/120Hz DAR: 16:9 PAR: 16:9 1:1",
    "1280x720p@119.88/120Hz DAR: 16:9 PAR: 16:9 1:1",
    "720x480p@119.88/120Hz DAR: 4:3 PAR: 4:3 8:9",
    "720x480p@119.88/120Hz DAR: 16:9 PAR: 16:9 32:27",
    "720(1440)x480i@119.88/120Hz DAR: 4:3 PAR: 4:3 8:9",
    "720(1440)x480i@119.88/120Hz DAR: 16:9 PAR: 16:9 32:27",
    "720x576p@200Hz DAR: 4:3 PAR: 4:3 16:15",
    "720x576p@200Hz DAR: 16:9 PAR: 16:9 64:45",
    "720(1440)x576i@200Hz DAR: 4:3 PAR: 4:3 16:15",
    "720(1440)x576i@200Hz DAR: 16:9 PAR: 16:9 64:45",
    "720x480p@239.76/240Hz DAR: 4:3 PAR: 4:3 8:9",
    "720x480p@239.76/240Hz DAR: 16:9 PAR: 16:9 32:27",
    "720(1440)x480i@239.76/240Hz DAR: 4:3 PAR: 4:3 8:9",
    "720(1440)x480i@239.76/240Hz DAR: 16:9 PAR: 16:9 32:27",
    "1280x720p@23.97Hz/24Hz DAR: 16:9 PAR: 16:9 1:1",
    "1280x720p@25Hz DAR: 16:9 PAR: 16:9 1:1",
    "1280x720p@29.97Hz/30Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080p@119.88/120Hz DAR: 16:9 PAR: 16:9 1:1",
    "1920x1080p@100Hz DAR: 16:9 PAR: 16:9 1:1",
};

