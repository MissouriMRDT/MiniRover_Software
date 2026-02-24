#include "hardware.h"
#include "net.h"
#include "tft.h"
#include "web.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <unistd.h>
#include "driver/uart.h"

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
  ledc_fade_func_install(0);
  servo_control_init();

  set_wheel_speed(0, 0);
  sleep(5);

  ESP_LOGI(TAG_MAIN, "Start of While loop");
  while (true)
  {
    set_wheel_speed(INT16_MAX, INT16_MAX);
    set_wheel_speed(0, 0);
    set_wheel_speed(INT16_MIN, INT16_MIN);
    set_wheel_speed(0, 0);

    int16_t duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEFT_WHEELS_CHNL);
    ESP_LOGI(TAG_MAIN, "duty_is: %d", duty);
    // adc_init();
    // set_servo_positions(0, 0, 0);
    // sleep(1);
    // set_servo_positions(UINT16_MAX, 0, 0);
    // sleep(1);
    // set_servo_positions(UINT16_MAX / 2, 0, 0);
    // sleep(1);
    // set_servo_positions(UINT16_MAX / 3, 0, 0);
    // sleep(1);
  }

  // int64_t next = esp_timer_get_time() + 5000000;
  // ESP_LOGI("main", "start");
  // while (esp_timer_get_time() < next)
  // {
  //   ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1 << SOC_LEDC_TIMER_BIT_WIDTH) * 0.5));
  //   ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  //   vTaskDelay(100 / portTICK_PERIOD_MS);
  // }

  // ESP_LOGI("main", "move");
  // while (true)
  // {
  //   ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1 << SOC_LEDC_TIMER_BIT_WIDTH) * 0.9));
  //   ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  //   vTaskDelay(100 / portTICK_PERIOD_MS);
  // }

  wifi_init_softap();
  tft_init();
  tft_draw_image(1, pixels);
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  tft_draw_image(0, pixels);

  // Will not return.
  webserver();
}
