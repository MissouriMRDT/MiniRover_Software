#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "hardware.h"
#include "net.h"
#include "tft.h"
#include "web.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <unistd.h>

static const char *TAG_MAIN = "main.c";

void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG_MAIN, "ESP_WIFI_MODE_AP");
  pins_init();
  motor_control_init();
  set_wheel_speed(0, 0);
  servo_control_init();
  adc_init();
  wifi_init_softap();
  vTaskDelay(10000);
  tft_init();
  tft_draw_image(1, pixels);
  tft_draw_image(0, pixels);
  // Will not return.

  xTaskCreate(webserver, "webserver", 8192, NULL, 0, NULL);
  xTaskCreate(control, "control", 4096, NULL, 0, NULL);

}
