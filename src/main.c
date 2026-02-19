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
  ESP_LOGI(TAG_MAIN, "Start of While loop");
  while (true)
  {
    set_wheel_speed(0, 0);
    int duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEFT_WHEELS_CHNL);
    ESP_LOGI(TAG_MAIN, "duty is:, %d", duty);
    // set_servo_positions(1000, 1000, 1000);
    // sleep(1);
    // set_servo_positions(-1000, -1000, -1000);
    // sleep(1);
  }

  /*
  1000us full reverse, 1500 stop 2000 full forward

  int16_t - -> +
  set_wheel_speed(int16_t web_speed)
  {
    pulse_width = conversion(web_speed) (in us from 1000us to 2000us)
    set_pulse_width(pulse_width)
  }

  uint16 0 -> max
  set_arm_targets(uint16 uint16 uint16)
  {
    dutyx
    dutyj2
    dutyj3 = conv(web_angle) (a duty_cycle from deci% or 0 to uint16t_MAX)
    set_pwm(duty)....

  }
  */

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

  /*wifi_init_softap();
  tft_init();
  tft_draw_image(1, pixels);
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  tft_draw_image(0, pixels);

  // Will not return.
  webserver();*/
}
