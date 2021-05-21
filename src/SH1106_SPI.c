#include "SH1106_SPI.h"
#include <stdlib.h>

#ifdef __AVR__
#include "ASCII_AVR.h"
#endif

typedef enum
{
    DATA,
    CMD
} SEND_MODE_t;

void send(SH1106_SPI_t* context, uint8_t data, SEND_MODE_t mode)
{
    if(mode == CMD)
        context->set_pin(context->dc_pin, 0);
    else if(mode == DATA)
        context->set_pin(context->dc_pin, 1);
    context->set_pin(context->cs_pin, 0);
    context->transfer(data);
    context->set_pin(context->cs_pin, 1);
}

void goto_xy(SH1106_SPI_t* context, point_t pt)
{
    send(context, SET_PAGE + pt.y, CMD);
    send(context, SET_LOWER_ADDRESS & pt.x, CMD);
    send(context, SET_HIGHER_ADDRESS | (pt.x >> 4), CMD);
}

SH1106_SPI_t* create_SH1106_SPI(uint8_t width, 
                                uint8_t height, 
                                uint16_t dc_pin,
                                uint16_t cs_pin,
                                uint16_t reset_pin,
                                pin_mode_t pin_mode,
                                set_pin_t set_pin,
                                transfer_t transfer)
{
    SH1106_SPI_t* display = (SH1106_SPI_t*)malloc(sizeof(SH1106_SPI_t));
    if(display == NULL)
    {
        return display;
    }
    uint16_t page_height = ((uint16_t)height + 8 - 1) / 8;
    display->fb_size = (uint16_t)width * page_height;
    display->framebuffer = (uint8_t*)malloc(display->fb_size);
    if(display->framebuffer == NULL)
    {
        free(display);
        display = NULL;
        return display;
    }
    //*display = {.width = width, .height = height, .cs_pin = cs_pin, .dc_pin = dc_pin, reset_pin = reset_pin, .pin_mode = pin_mode, .set_pin = set_pin, .transfer = transfer};
    display->width = width;
    display->height = height;
    display->cs_pin = cs_pin;
    display->dc_pin = dc_pin;
    display->reset_pin = reset_pin;
    
    display->pin_mode = pin_mode;
    display->set_pin = set_pin;
    display->transfer = transfer;

    display->pin_mode(cs_pin, 1);
    display->pin_mode(dc_pin, 1);
    display->pin_mode(reset_pin, 1);

    display->set_pin(cs_pin, 1);
    display->set_pin(dc_pin, 1);
    display->set_pin(reset_pin, 1);

    send(display, DISPLAY_OFF, CMD);
    send(display, SET_LOWER_ADDRESS, CMD);
    send(display, SET_HIGHER_ADDRESS, CMD);
    send(display, SET_START_LINE, CMD);
    send(display, SET_PAGE, CMD);
    send(display, SET_CONSRAST, CMD);
    send(display, 0x80, CMD);
    send(display, SEGMENT_REMAP, CMD);
    send(display, NON_INVERTED_DISPLAY, CMD);
    send(display, SET_MULTIPLEX, CMD);
    send(display, 0x3F, CMD);
    send(display, ENABLE_CHARGEPUMP, CMD);
    send(display, 0x8B, CMD);
    send(display, 0x00, CMD);
    send(display, SET_COM_DIR, CMD);
    send(display, SET_OFFSET, CMD);
    send(display, 0x00, CMD);
    send(display, SET_OSC_DIVFREQ, CMD);
    send(display, 0x80, CMD);
    send(display, SET_PRECHARGE_PER, CMD);
    send(display, 0x1F, CMD);
    send(display, SET_COM_PADS, CMD);
    send(display, 0x12, CMD);
    send(display, SET_VCOM_LEVEL, CMD);
    send(display, 0x40, CMD);
    send(display, DISPLAY_ON, CMD);

    for(uint32_t i = 0; i < display->fb_size; ++i)
    {
        display->framebuffer[i] = 0;
    }

    return display;
}

void draw_str(SH1106_SPI_t* context, const char* str, point_t pt)
{
    uint16_t i = 0;
    uint8_t page = (pt.y != 0) ? pt.y/8 : 0;
    while(str[i] != '\0')
    {
        char c = str[i];
        if(c < 0x20 || c > 0x7F)
            break;
        c -= 0x20;
        if(pt.x > 123)
        {
            ++page;
            pt.x = 0;
        }
            
        if(page > 7)
            break;

        if(page == ((pt.y != 0) ? pt.y/8 : 0))
        {
            for(uint8_t j = 0; j < 5; ++j)
            {
                context->framebuffer[page*context->width + pt.x] = read_ascii_byte(c, j) << (pt.y%8);
                if(pt.y%8 != 0)
                {
                    context->framebuffer[(page+1)*context->width + pt.x] = read_ascii_byte(c, j) >> (8-(pt.y%8));
                }
                ++pt.x;
            }
        } else {
            for(uint8_t j = 0; j < 5; ++j)
            {
                if(pt.y%8 != 0)
                {
                    context->framebuffer[(page+1)*context->width + pt.x] = read_ascii_byte(c, j) >> (8-(pt.y%8));
                    context->framebuffer[page*context->width + pt.x] &= ~(0xFF << (pt.y%8));
                }
                context->framebuffer[page*context->width + pt.x] |= read_ascii_byte(c, j) << (pt.y%8);
                ++pt.x;
            }
        }
        ++i;
    }
}

void draw_shape(SH1106_SPI_t* context, uint8_t shape[], point_t pt, uint8_t stride, uint8_t shape_len)
{

}

void update(SH1106_SPI_t* context)
{
    point_t pt = {.x = 2, .y = 0};
    goto_xy(context, pt);
    for(uint32_t i = 0; i < context->fb_size; ++i)
    {
        if(i % context->width == 0 && i != 0)
        {
            ++pt.y;
            goto_xy(context, pt);
        }
        send(context, context->framebuffer[i], DATA);
    }
}