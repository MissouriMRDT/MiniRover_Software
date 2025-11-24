#ifndef _TFT_H_
#define _TFT_H_

#include <esp_attr.h>
#include <stdint.h>

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

// To speed up transfers, every SPI transfer sends a bunch of lines. This define
// specifies how many. More means more memory use, but less overhead for setting
// up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 60
#define PIXELS_LENGTH LCD_WIDTH *(PARALLEL_LINES)
#define PIXELS_BYTES PIXELS_LENGTH * sizeof(uint16_t)

static const char *TAG_TFT = "main.c";

#define LOGO_COLORS 3
extern const char FILE_LOGO_START[] asm("_binary_logo_bin_start");
extern const char FILE_LOGO_END[] asm("_binary_logo_bin_end");
extern const char FILE_LOGO_START1[] asm("_binary_logo_bmp_start");
extern const char FILE_LOGO_END1[] asm("_binary_logo_bmp_end");

void tft_init(void);
void tft_send_image_part(uint8_t part, uint16_t pixels[PIXELS_LENGTH]);
void tft_draw_logo(uint16_t pixels[PIXELS_LENGTH]);

#endif