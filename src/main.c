#include "hardware.h"
#include "net.h"
#include "tft.h"
#include "web.h"
#include "driver/gpio.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <unistd.h>

static const char *TAG_MAIN = "main.c";

void app_main(void)
{
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG_MAIN, "ESP_WIFI_MODE_AP");
  motor_control_init();
  pins_init();

  motor_control_set(0, 500, 0, 0, 0);
  sleep(1);
  ledc_fade_func_install(0);
  while (1)
  {
    set_fade(FRONT_RIGHT_WHEEL_CHNL, 1000, 4, 250);
    uint32_t duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, 2);
    ESP_LOGI(TAG_MAIN, "Ramping_Testing %d", duty);
  }
// while (1)
// {
//   for (int i = 500; i < 1000; i += 50)
//   {
//     motor_control_set(0, i, 0, 0, 0);
//     sleep(1);
//   }
//   motor_control_set(0, 0, 0, 0, 0);
//   sleep(2);
// }
#include "esp_intr_alloc.h"
  return;

  wifi_init_softap();
  tft_init();
  tft_draw_image(1, pixels);
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  tft_draw_image(0, pixels);

  // Will not return.
  webserver();
}
