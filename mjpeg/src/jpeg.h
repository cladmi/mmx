/* vim:set ts=3: */
#ifndef __JPEG_H__
#define __JPEG_H__

#include <stdint.h>

#define MCU_sx 8
#define MCU_sy 8

#define HUFF_DC     0
#define HUFF_AC     1
#define HUFF_EOB    0x00
#define HUFF_ZRL    0xF0

#define MAX_SIZE(class) ((class)?162:14)        /* Memory size of * HTables */

#define M_SOF0  0xc0            /* Baseline DCT */
#define M_SOF1  0xc1            /* Extended sequential DCT */
#define M_SOF2  0xc2            /* Progressive DCT */
#define M_SOF3  0xc3            /* Lossless (sequential) */
#define M_DHT   0xc4            /* Define Huffman tables */
#define M_SOF5  0xc5            /* Differential sequential DCT */
#define M_SOF6  0xc6            /* Differential progressive DCT */
#define M_SOF7  0xc7            /* Differential lossless */
#define M_JPG   0xc8            /* JPEG extensions */
#define M_SOF9  0xc9            /* Extended sequential DCT */
#define M_SOF10 0xca            /* Progressive DCT */
#define M_SOF11 0xcb            /* Lossless (sequential) */
#define M_DAC   0xcc            /* Define arithmetic conditioning table */
#define M_SOF13 0xcd            /* Differential sequential DCT */
#define M_SOF14 0xce            /* Differential progressive DCT */
#define M_SOF15 0xcf            /* Differential lossless */
#define M_RST0  0xd0            /* Restart */
#define M_RST1  0xd1            /* Restart */
#define M_RST2  0xd2            /* Restart */
#define M_RST3  0xd3            /* Restart */
#define M_RST4  0xd4            /* Restart */
#define M_RST5  0xd5            /* Restart */
#define M_RST6  0xd6            /* Restart */
#define M_RST7  0xd7            /* Restart */
#define M_SOI   0xd8            /* Start of image */
#define M_EOI   0xd9            /* End Of Image */
#define M_SOS   0xda            /* Start Of Scan */
#define M_DQT   0xdb            /* Define quantization tables */
#define M_DNL   0xdc            /* Define number of lines */
#define M_DRI   0xdd            /* Define restart interval */
#define M_DHP   0xde            /* Define hierarchical progression */
#define M_EXP   0xdf            /* Expand reference image(s) */
#define M_APP0  0xe0            /* Application marker, used for JFIF */
#define M_APP1  0xe1            /* Application marker */
#define M_APP2  0xe2            /* Application marker */
#define M_APP3  0xe3            /* Application marker */
#define M_APP4  0xe4            /* Application marker */
#define M_APP5  0xe5            /* Application marker */
#define M_APP6  0xe6            /* Application marker */
#define M_APP7  0xe7            /* Application marker */
#define M_APP8  0xe8            /* Application marker */
#define M_APP9  0xe9            /* Application marker */
#define M_APP10 0xea            /* Application marker */
#define M_APP11 0xeb            /* Application marker */
#define M_APP12 0xec            /* Application marker */
#define M_APP13 0xed            /* Application marker */
#define M_APP14 0xee            /* Application marker, used by Adobe */
#define M_APP15 0xef            /* Application marker */
#define M_JPG0  0xf0            /* Reserved for JPEG extensions */
#define M_JPG13 0xfd            /* Reserved for JPEG extensions */
#define M_COM   0xfe            /* Comment */
#define M_TEM   0x01            /* Temporary use */
#define M_ERROR 0x100           /* Dummy marker, internal use only */
#define M_SMS   0xff            /* Start Marker Segment */

#define M_MARKER(m)             (0xff << 8 | m)
#define UNSUPPORTED_MARKER(m) printf("marker 0x%04x not implemented\n",m)

#define GOOD_NODE_FLAG          0x100
#define GOOD_LEAF_FLAG          0x200
#define BAD_LEAF_FLAG           0x300
#define SPECIAL_FLAG            0x000
#define HUFF_FLAG_MSK           0x300

#define HUFF_FLAG(c)            ((c) & HUFF_FLAG_MSK)
#define HUFF_VALUE(c)           ((unsigned char)( (c) & (~HUFF_FLAG_MSK) ))

typedef unsigned short jpeg_marker_t;

typedef struct _huff_table_t {
	int16_t code ;
	int8_t  value ;
	int8_t is_elt ;
	struct _huff_table_t *parent ;
   struct _huff_table_t *left ;
	struct _huff_table_t *right ;
} huff_table_t;

//typedef struct {
//    int32_t ValPtr[16];
//    int32_t MinCode[16];
//    int32_t MaxCode[16];
//    uint8_t *table;
//} huff_table_t;

typedef struct {
    uint8_t bit_count;
    uint8_t window;
    int32_t pred[3];
    huff_table_t *table[2][3];
} scan_desc_t;

typedef struct __attribute__ ((__packed__)) {
        uint16_t length;        /* Length of APP0 Field */
        char identifier[5];     /* "JFIF" (zero terminated) Id * String */
        uint8_t version[2];     /* JFIF Format Revision */
        uint8_t units;          /* Units used for Resolution */
        uint16_t xdensity;      /* Horizontal Resolution */
        uint16_t ydensity;      /* Vertical Resolution */
        uint8_t xthumbnail;     /* Horizontal Pixel Count */
        uint8_t ythumbnail;     /* Vertical Pixel Count */
} jfif_header_t;

typedef struct __attribute__ ((__packed__)) {
        uint16_t length;        /* Length of DQT section */
        uint8_t  pre_quant;     /* 4bits high : precision (0=8bits, * 1=16bits) */
} DQT_section_t;

typedef struct __attribute__ ((__packed__)) {
        uint8_t index;          /* Component index */
        uint8_t HV;             /* H(4bits high) and V(4bits low) sampling * factors */
        uint8_t q_table;        /* Quantization table associated */
} SOF_component_t;

typedef struct __attribute__ ((__packed__)) {
        uint16_t length;        /* Length of SOF section */
        uint8_t data_precision; /* Data bit precision (usually 8, * for baseline JPEG) */
        uint16_t height;        /* Image height in pixels */
        uint16_t width;         /* Image width in pixels */
        uint8_t n;              /* n = Number of components (eg. 3 for * RGB, 1 for Grayscale) */
} SOF_section_t;

typedef struct __attribute__ ((__packed__)) {
        uint16_t length;        /* Length in byte of this section, * including this data. */
        uint8_t huff_info;      /* Huffman Table information : */
                                /* bit 0..3 : index (0..3, otherwise error) */
                                /* bit 4 : type (0=DC, 1=AC) */
                                /* bit 5..7 : not used, must be 0 */
} DHT_section_t;

typedef struct __attribute__ ((__packed__)) {
        uint8_t index;
        uint8_t acdc;
} SOS_component_t;

typedef struct __attribute__ ((__packed__)) {
        uint16_t length;        /* Length in byte of this section, * including this data. */
        uint8_t   n;            /* N = Number of components */
} SOS_section_t;

#endif                          // __JPEG_H__
