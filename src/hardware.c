#include "hardware.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "stdint.h"
#include <esp_log.h>
#include <esp_timer.h>

bool estop_get(void) {
  // TODO: return true if estop depressed
  return false;
}

void buzzer_set(bool on) {
  // TODO: turn on and off buzer
  ESP_LOGI("hardware.c", "buzzer %s", on ? "on" : "off");
}

void esc_enabled_set(bool enabled) {
  // TODO: enable and disable ESC
  ESP_LOGI("hardware.c", "esc %s", enabled ? "enabled" : "disabled");
}

void motor_control_init(void) {
  // TODO: Initialize timers and channels for every PWM output.
  /*ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = SOC_LEDC_TIMER_BIT_WIDTH,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 4000, // Set output frequency at 4 kHz
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = PIN_DRIVE_FRONT_RIGHT,
      .duty = 0, // Set duty to 0%
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));*/
  ESP_LOGI("hardware.c", "motor_control_init");
}

void motor_control_set(int16_t left, int16_t right, uint16_t x, uint16_t j2,
                       uint16_t j3) {
  // TODO: Set PWM duty cycles.
  // scale ledc_set_duty parameter 3 from [INT16_MIN, INT16_MAX] or
  // [0, UINT16_MAX] to [0, (1 << SOC_LEDC_TIMER_BIT_WIDTH) - 1]

  // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
  // ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  ESP_LOGI("hardware.c", "left %d right %d x %u j2 %u j3 %u", left, right, x,
           j2, j3);
}

void current_sense_get(float *esc, float *cell1, float *cell2, float *cell3) {
  // TODO: Read, calculate, and return ESC and cell sense values.
  int64_t now = esp_timer_get_time();
  *esc = (float)(now % 1000000) / 1000000 + 0.1;
  *cell1 = (float)(now % 1000000) / 1000000 + 0.2;
  *cell2 = (float)(now % 1000000) / 1000000 + 0.3;
  *cell3 = (float)(now % 1000000) / 1000000 + 0.4;
}
