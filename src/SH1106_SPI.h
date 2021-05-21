#ifndef SH1106_SPI_H
#define SH1106_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DISPLAY_OFF 0xAE
#define DISPLAY_ON 0xAF

#define SET_CONSRAST 0x81
#define ENABLE_CHARGEPUMP 0xAD

#define SET_PAGE 0xB0
#define SET_LOWER_ADDRESS 0x0F
#define SET_HIGHER_ADDRESS 0x10

#define SET_OUTPUT_VOLT 0x00
#define SET_START_LINE 0x40

#define SEGMENT_REMAP 0xA1
#define NON_INVERTED_DISPLAY 0xA6
#define INVERTED_DISPLAY 0xA7

#define SET_MULTIPLEX 0xA8

#define SET_COM_DIR 0xC8
#define SET_OFFSET 0xD3

#define SET_OSC_DIVFREQ 0xD5
#define SET_PRECHARGE_PER 0xD9

#define SET_COM_PADS 0xDA
#define SET_VCOM_LEVEL 0xDB

typedef struct
{
    uint8_t x, y;
} point_t;

typedef void (*pin_mode_t)(uint8_t pin, uint8_t mode);
typedef void (*set_pin_t)(uint8_t pin, uint8_t value);
typedef uint8_t (*transfer_t)(uint8_t data);

typedef struct
{
    uint8_t* framebuffer;
    uint16_t fb_size;
    uint8_t width, height;
    uint16_t dc_pin, cs_pin, reset_pin;
    pin_mode_t pin_mode;
    set_pin_t set_pin;
    transfer_t transfer;
} SH1106_SPI_t;

SH1106_SPI_t* create_SH1106_SPI(uint8_t width, 
                                uint8_t height,
                                uint16_t dc_pin,
                                uint16_t cs_pin,
                                uint16_t reset_pin,
                                pin_mode_t pin_mode,
                                set_pin_t set_pin,
                                transfer_t transfer);

void draw_str(SH1106_SPI_t* context, const char* str, point_t pt);

void draw_shape(SH1106_SPI_t* context, uint8_t shape[], point_t pt, uint8_t stride, uint8_t shape_len);

void update(SH1106_SPI_t* context);

#ifdef __cplusplus
}
#endif
#endif