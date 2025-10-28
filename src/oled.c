/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "oled.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

static const char *TAG_OLED = "oled.c";

#define LCD_WIDTH 128
#define LCD_HEIGHT 32

uint8_t pattern1[512] = {
    115, 5,   160, 6,   0,   51,  154, 233, 254, 201, 115, 200, 23,  118, 174,
    65,  207, 48,  94,  167, 125, 19,  109, 13,  54,  75,  122, 147, 223, 93,
    179, 219, 41,  195, 98,  120, 237, 226, 169, 12,  213, 123, 245, 84,  44,
    93,  219, 89,  185, 186, 103, 123, 11,  16,  76,  15,  204, 116, 114, 150,
    63,  173, 130, 229, 10,  211, 102, 83,  64,  208, 140, 128, 13,  102, 62,
    56,  198, 125, 179, 83,  64,  105, 94,  92,  205, 37,  38,  53,  220, 251,
    83,  238, 79,  247, 227, 38,  45,  253, 17,  110, 13,  43,  107, 114, 97,
    110, 104, 93,  91,  20,  40,  202, 189, 191, 239, 161, 60,  159, 91,  113,
    75,  240, 121, 30,  181, 239, 97,  2,   228, 79,  227, 57,  94,  249, 111,
    162, 229, 24,  243, 56,  149, 92,  93,  70,  223, 142, 84,  85,  223, 243,
    187, 44,  183, 71,  246, 127, 36,  193, 112, 241, 191, 188, 166, 181, 26,
    192, 65,  34,  19,  34,  133, 70,  143, 33,  139, 29,  249, 198, 209, 223,
    216, 155, 138, 220, 179, 221, 233, 175, 218, 128, 147, 247, 12,  97,  29,
    71,  7,   179, 234, 153, 238, 85,  143, 74,  198, 206, 221, 4,   59,  171,
    192, 204, 210, 48,  13,  22,  97,  222, 15,  112, 255, 178, 158, 137, 244,
    51,  185, 24,  49,  15,  196, 29,  191, 123, 44,  41,  189, 235, 117, 161,
    206, 188, 127, 20,  205, 187, 226, 49,  65,  100, 20,  23,  97,  68,  35,
    90,  89,  223, 231, 57,  39,  139, 12,  174, 199, 33,  25,  5,   116, 219,
    208, 145, 130, 113, 85,  252, 98,  108, 242, 211, 183, 222, 6,   204, 172,
    184, 90,  158, 116, 55,  46,  252, 120, 90,  108, 125, 237, 11,  179, 254,
    241, 16,  90,  60,  68,  241, 114, 91,  120, 222, 255, 29,  157, 157, 202,
    229, 196, 0,   119, 131, 207, 4,   47,  229, 181, 242, 227, 208, 217, 163,
    123, 248, 43,  138, 43,  154, 58,  93,  13,  160, 113, 50,  59,  72,  228,
    18,  175, 136, 158, 246, 14,  37,  169, 13,  184, 246, 209, 181, 143, 240,
    169, 64,  132, 135, 139, 19,  232, 203, 252, 6,   80,  185, 114, 19,  234,
    134, 221, 255, 98,  126, 69,  87,  118, 130, 238, 78,  59,  16,  196, 33,
    207, 182, 195, 215, 152, 36,  140, 71,  97,  76,  139, 196, 22,  21,  190,
    97,  81,  178, 176, 47,  42,  213, 215, 7,   243, 240, 65,  8,   150, 203,
    220, 111, 185, 116, 211, 148, 210, 82,  93,  27,  35,  110, 12,  113, 94,
    24,  148, 140, 58,  26,  34,  207, 138, 32,  144, 53,  170, 166, 102, 44,
    135, 18,  79,  152, 8,   237, 152, 104, 43,  158, 74,  132, 173, 5,   166,
    185, 228, 245, 74,  76,  101, 146, 157, 137, 82,  156, 165, 20,  251, 197,
    0,   5,   198, 222, 149, 72,  16,  95,  183, 225, 188, 58,  14,  230, 248,
    149, 2,   235, 250, 69,  199, 92,  169, 114, 146, 112, 57,  69,  83,  32,
    82,  108};

uint8_t pattern2[512] = {
    15,  148, 52,  110, 211, 255, 113, 204, 229, 136, 2,   183, 32,  135, 24,
    225, 144, 42,  128, 173, 14,  0,   115, 169, 143, 245, 32,  239, 210, 162,
    21,  77,  47,  23,  252, 70,  56,  212, 90,  196, 203, 86,  177, 41,  254,
    44,  42,  64,  64,  224, 240, 77,  96,  184, 64,  93,  55,  230, 73,  158,
    14,  212, 147, 248, 194, 206, 112, 223, 39,  38,  57,  169, 126, 28,  219,
    74,  180, 51,  207, 167, 230, 149, 245, 211, 77,  190, 86,  114, 198, 203,
    27,  114, 44,  189, 139, 185, 151, 209, 81,  120, 185, 186, 160, 110, 148,
    222, 176, 114, 136, 81,  184, 62,  115, 170, 95,  206, 255, 96,  4,   21,
    145, 3,   164, 194, 82,  234, 38,  13,  140, 243, 219, 34,  201, 255, 58,
    77,  177, 139, 35,  83,  91,  13,  11,  169, 11,  242, 161, 170, 60,  4,
    231, 92,  107, 238, 183, 80,  140, 6,   111, 210, 108, 145, 51,  190, 224,
    81,  209, 155, 205, 185, 247, 217, 242, 96,  120, 140, 231, 252, 243, 21,
    162, 67,  0,   98,  197, 130, 254, 0,   179, 247, 34,  167, 42,  83,  218,
    132, 145, 8,   180, 5,   88,  47,  132, 76,  207, 180, 163, 152, 181, 77,
    234, 33,  1,   239, 100, 249, 59,  63,  224, 83,  49,  93,  244, 57,  253,
    108, 49,  145, 3,   83,  30,  169, 230, 103, 231, 120, 247, 36,  151, 103,
    74,  125, 63,  202, 125, 163, 72,  44,  8,   234, 136, 108, 42,  20,  244,
    55,  68,  22,  57,  54,  213, 254, 66,  163, 209, 244, 18,  80,  203, 71,
    222, 65,  98,  156, 224, 191, 154, 164, 70,  54,  210, 76,  228, 61,  182,
    72,  78,  123, 165, 164, 226, 35,  14,  201, 74,  218, 158, 245, 70,  191,
    172, 53,  237, 181, 48,  235, 175, 41,  183, 111, 241, 21,  37,  243, 149,
    140, 174, 46,  235, 230, 105, 204, 72,  165, 4,   1,   173, 205, 121, 201,
    222, 168, 20,  244, 157, 195, 188, 10,  29,  178, 235, 101, 107, 186, 114,
    188, 255, 158, 85,  111, 163, 153, 248, 0,   249, 11,  199, 149, 51,  215,
    23,  165, 34,  127, 215, 36,  193, 38,  57,  99,  168, 167, 197, 253, 11,
    124, 210, 90,  7,   184, 224, 236, 109, 89,  175, 155, 82,  63,  167, 173,
    37,  36,  193, 7,   193, 48,  118, 106, 206, 144, 31,  76,  118, 107, 171,
    60,  217, 17,  54,  234, 27,  202, 171, 2,   219, 29,  56,  24,  15,  76,
    81,  185, 250, 72,  125, 180, 111, 26,  97,  130, 164, 210, 48,  37,  102,
    99,  247, 139, 74,  9,   27,  18,  243, 211, 12,  122, 12,  85,  218, 136,
    133, 169, 9,   164, 8,   1,   112, 87,  114, 194, 144, 237, 14,  104, 32,
    244, 236, 194, 66,  246, 28,  49,  206, 76,  141, 77,  246, 53,  207, 58,
    121, 196, 121, 157, 121, 121, 163, 195, 47,  174, 69,  38,  96,  20,  119,
    139, 146, 10,  226, 211, 88,  188, 219, 244, 123, 97,  111, 139, 164, 44,
    75,  34};

void draw(esp_lcd_panel_handle_t panel_handle, int x, int y, int w, int h,
          uint8_t pattern[]) {
  ESP_ERROR_CHECK(
      esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + w, y + h, pattern));
}

void oled_main(void) {
  ESP_LOGI(TAG_OLED, "Initialize I2C bus");
  i2c_master_bus_handle_t i2c_bus = NULL;
  i2c_master_bus_config_t bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .i2c_port = 0,
      .sda_io_num = 11, // 2
      .scl_io_num = 12, // 3
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

  ESP_LOGI(TAG_OLED, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t io_config = {
      .dev_addr = 0x3C,
      .scl_speed_hz = (400 * 1000),
      .control_phase_bytes = 1, // According to SSD1306 datasheet
      .lcd_cmd_bits = 8,        // According to SSD1306 datasheet
      .lcd_param_bits = 8,      // According to SSD1306 datasheet
      .dc_bit_offset = 6,       // According to SSD1306 datasheet
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

  ESP_LOGI(TAG_OLED, "Install SSD1306 panel driver");
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .bits_per_pixel = 1,
      .reset_gpio_num = -1,
  };
  esp_lcd_panel_ssd1306_config_t ssd1306_config = {
      .height = LCD_HEIGHT,
  };
  panel_config.vendor_config = &ssd1306_config;
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  int count = 0;
  int last_time = esp_timer_get_time();
  while (true) {
    if (count % 2 == 0)
      draw(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, pattern1);
    else
      draw(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, pattern2);

    if (count++ % 20 == 0) {
      int64_t time = esp_timer_get_time();
      printf("%lld\n", time - last_time);
      last_time = time;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  /*const uint8_t pattern[][16] = {
      {0x00, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x00, 0x00, 0x7E, 0x42,
  0x42, 0x42, 0x42, 0x7E, 0x00}, {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42,
  0x81, 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81}};

  for (int i = 0; i < LCD_WIDTH / 16; i++) {
    for (int j = 0; j < LCD_HEIGHT / 8; j++) {
      ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, i * 16, j * 8,
                                                i * 16 + 16, j * 8 + 8,
                                                pattern[i & 0x01]));
    }
  }*/
  /*uint8_t fill16x8[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  for (uint x = 0; x < LCD_WIDTH / 16; x++) {
    for (uint y = 0; y < LCD_HEIGHT / 8; y++) {
      ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
          panel_handle, x * 16, y * 8, x * 16 + 16, y * 8 + 8, fill16x8));
    }
  }*/

  // vTaskDelay(2000 / portTICK_PERIOD_MS);

  /*uint8_t empty16x8[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (uint x = 0; x < LCD_WIDTH / 16; x++) {
    for (uint y = 0; y < LCD_HEIGHT / 8; y++) {
      ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
          panel_handle, x * 16, y * 8, x * 16 + 16, y * 8 + 8, empty16x8));
    }
  }*/
}