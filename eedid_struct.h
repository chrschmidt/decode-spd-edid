#pragma once

#include <stdint.h>

#pragma pack(1)

struct detailed_timing_t {
/* 0 */
    uint16_t pixel_clock;
    uint8_t horz_act_lo;
    uint8_t horz_blank_lo;
    uint8_t horz_act_blank_hi;          /* upper nibble: active, lower: blank */
    uint8_t vert_act_lo;
    uint8_t vert_blank_lo;
    uint8_t vert_act_blank_hi;          /* upper nibble: active, lower: blank */
/* 8 */
    uint8_t horz_sync_offset;
    uint8_t horz_sync_width;
    uint8_t vert_sync_offset_width;     /* upper nibble: offset, lower: width */
    uint8_t sync_hi;                    /* 2bit hsync ofs, wid, vsync ofs, wid*/
    uint8_t horz_img_size_lo;
    uint8_t vert_image_size_lo;
    uint8_t image_size_hi;              /* upper nibble: horz, lower: vert */
    uint8_t horz_border;
/* 16 */
    uint8_t vert_border;
    uint8_t flags;
};

struct device_descriptor_t {
    uint16_t flag1;
    uint8_t flag2;
    uint8_t tag;
    uint8_t flag3;
    uint8_t data[13];
};

struct gtf_secondary_curve_t {
    uint8_t reserved;
    uint8_t start_break_freq;
    uint8_t c;
    uint16_t m;
    uint8_t k;
    uint8_t j;
};

struct cvt_support_t {
    uint8_t cvt_version;
    uint8_t extra_clock;
    uint8_t max_lines_lo;
    uint8_t supported_ar;
    uint8_t preferred_ar_blanking;
    uint8_t scaling;
    uint8_t preferred_refresh;
};

struct range_limits_t {
    uint32_t header;
    uint8_t offsets;
    uint8_t min_vert_rate;
    uint8_t max_vert_rate;
    uint8_t min_horz_rate;
    uint8_t max_horz_rate;
    uint8_t max_pixel_clock;
    uint8_t video_timing_support;
    union {
        struct gtf_secondary_curve_t gtf;
        struct cvt_support_t cvt;
        uint8_t blank[7]; /* must be 0x0a 0x20 0x20 0x20 0x20 0x20 0x20 */
    };
};

union eighteen_bytes_descriptor_t {
    struct detailed_timing_t timing;
    struct device_descriptor_t desc;
    struct range_limits_t limits;
};

struct eedid_t {
/* 8 bytes header */
    uint8_t header[8];
/* 10 bytes vendor & product information */
    uint16_t id_manufacturer_name;
    uint16_t id_product_code;
    uint32_t id_serial_number;
    uint8_t week_of_manufacture;
    uint8_t year_of_manufacture;
/* 2 bytes EDID structure version & revision */
    uint8_t edid_version;
    uint8_t edid_revision;
/* 5 bytes basic display parameters & features */
    uint8_t video_input_definition;
    uint8_t horz_size_ar;
    uint8_t vert_size_ar;
    uint8_t gamma;
    uint8_t features;
/* 10 bytes color characteristics */
    uint8_t rg_lo;
    uint8_t bw_lo;
    uint8_t rx_hi;
    uint8_t ry_hi;
    uint8_t gx_hi;
    uint8_t gy_hi;
    uint8_t bx_hi;
    uint8_t by_hi;
    uint8_t wx_hi;
    uint8_t wy_hi;
/* 3 bytes established timings support */
    uint16_t established_timings;
    uint8_t manufacturer_timings;
/* 16 bytes standard timings */
    uint16_t standard_timings[8];
/* 4*18 bytes detailed timing / display descriptor */
    union eighteen_bytes_descriptor_t detailed_timings[4];
/* remainder */
    uint8_t extension_block_count;
    uint8_t checksum;
} eedid_t;

struct stdtimingentry_t {
    uint8_t stdid[2];
    char * name;
};

enum cea861_datatypes {
    cea_reserved = 0,
    cea_audio,
    cea_video,
    cea_vendor,
    cea_speaker,
    cea_vesa_dtc,
    cea_reserved_2,
    cea_extended
};

enum cea861_ext_datatypes {
    cea_video_caps = 0,
    cea_vendor_video,
    cea_vesa_ddd_information,
    cea_vesa_video,
    cea_hdmi_video,
    cea_colorimetry,
    cea_hdr_static_metadata,
    cea_hdr_dynamic_metadata,
    cea_video_format_preference = 13,
    cea_ycbcr_420_video_data,
    cea_ycbcr_420_capsmap,
    cea_misc_audio,
    cea_vendor_audio,
    cea_hdmi_audio,
    cea_room_configuration,
    cea_speaker_location,
    cea_infoframe = 32
};

enum cea861_audio_formats {
    cea_audio_from_header = 0,
    cea_audio_lpcm,
    cea_audio_ac3,
    cea_audio_mpeg1,
    cea_audio_mp3,
    cea_audio_mpeg2,
    cea_audio_aac_lc,
    cea_audio_dts,
    cea_audio_atrac,
    cea_audio_dsd,
    cea_audio_eac3,
    cea_audio_dtshd,
    cea_audio_dst,
    cea_audio_wmapro,
    cea_audio_extended,
    cea_audio_he_aac,
    cea_audio_he_aacv2,
    cea_audio_mpeg_surround };

struct eedid_ext_cea861 {
    uint8_t tag;
    uint8_t version;
    uint8_t _18b_start;
    uint8_t flags;
    uint8_t data[123];
    uint8_t checksum;
};

struct cea861_vendor {
    uint8_t id_lo;
    uint8_t id_mid;
    uint8_t id_hi;
};

struct cea861_vendor_hdmi {
    uint16_t phys_address;
    uint8_t video_flags;
    uint8_t max_tmds_clock;
    uint8_t latency_fields;
    uint8_t video_latency;
    uint8_t audio_latency;
    uint8_t interlaced_video_latency;
    uint8_t interlaced_audio_latency;
};
