#ifndef _WEB_H_
#define _WEB_H_

#include <esp_http_server.h>
#include <tft.h>

static const char *TAG_WEB = "web.c";

#define SEND_CHUNK_SIZE 8192

extern const char FILE_FONT_START[] asm("_binary_B612Mono_woff2_start");
extern const char FILE_FONT_END[] asm("_binary_B612Mono_woff2_end");
extern const char FILE_HTML_START[] asm("_binary_index_html_start");
extern const char FILE_HTML_END[] asm("_binary_index_html_end");
extern const char FILE_IMAGE_JS_START[] asm("_binary_images_js_start");
extern const char FILE_IMAGE_JS_END[] asm("_binary_images_js_end");
extern const char FILE_JS_START[] asm("_binary_main_js_start");
extern const char FILE_JS_END[] asm("_binary_main_js_end");
extern const char FILE_CSS_START[] asm("_binary_style_css_start");
extern const char FILE_CSS_END[] asm("_binary_style_css_end");

extern uint16_t pixels[PIXELS_LENGTH];

httpd_handle_t start_webserver(void);
#endif