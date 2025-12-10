#include "hardware.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "stdint.h"
#include "esp_timer.h"

bool estop_get(void)
{
  if (gpio_get_level(PIN_ESTOP) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void pins_init()
{
  gpio_config_t GPIO_config = {
      .pin_bit_mask = (1ULL << PIN_BUZZER) |
                      (1ULL << PIN_ARM_PWM_X) |
                      (1ULL << PIN_ARM_PWM_J2) |
                      (1ULL << PIN_ARM_PWM_J3) |
                      (1ULL << PIN_DRIVE_MID_LEFT_6) |
                      (1ULL << PIN_DRIVE_MID_RIGHT_5) |
                      (1ULL << PIN_DRIVE_FRONT_LEFT_4) |
                      (1ULL << PIN_DRIVE_BACK_LEFT_3) |
                      (1ULL << PIN_DRIVE_FRONT_RIGHT_2) |
                      (1ULL << PIN_DRIVE_BACK_RIGHT_1) |
                      (1ULL << PIN_CURRENT_ESC),
      .mode = GPIO_MODE_OUTPUT,              /*!< GPIO mode: set input/output mode                     */
      .pull_up_en = GPIO_PULLUP_DISABLE,     /*!< GPIO pull-up                                         */
      .pull_down_en = GPIO_PULLDOWN_DISABLE, /*!< GPIO pull-down                                       */
      .intr_type = GPIO_INTR_DISABLE,
  };

  gpio_config(&GPIO_config);

  GPIO_config.pin_bit_mask = (1ULL << PIN_CURRENT_CELL_1) |
                             (1ULL << PIN_CURRENT_CELL_2) |
                             (1ULL << PIN_CURRENT_CELL_3) |
                             (1ULL << PIN_CURRENT_ESC) |
                             (1ULL << PIN_ARM_ENCODER_X) |
                             (1ULL << PIN_ARM_ENCODER_J2) |
                             (1ULL << PIN_ARM_ENCODER_J3);
  GPIO_config.mode = GPIO_MODE_INPUT;               /*!< GPIO mode: set input/output mode                     */
  GPIO_config.pull_up_en = GPIO_PULLUP_DISABLE;     /*!< GPIO pull-up                                         */
  GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE; /*!< GPIO pull-down                                       */
  GPIO_config.intr_type = GPIO_INTR_DISABLE;

  gpio_config(&GPIO_config);

  GPIO_config.pin_bit_mask = (1ULL << PIN_ESTOP);
  GPIO_config.mode = GPIO_MODE_INPUT;               /*!< GPIO mode: set input/output mode                     */
  GPIO_config.pull_up_en = GPIO_PULLUP_ENABLE;      /*!< GPIO pull-up                                         */
  GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE; /*!< GPIO pull-down                                       */
  GPIO_config.intr_type = GPIO_INTR_DISABLE;

  gpio_config(&GPIO_config);
}

void buzzer_set(bool on)
{
  // Set time variable
  int64_t now = esp_timer_get_time();
  // check if buzzer on
  // if buzzer on: turn on
  // check time using module operator to set buzz pattern
  if (on)
  {
    if (now % 3000000 < 250000)
    {
      gpio_set_level(PIN_BUZZER, 1);
    }
    else if (now % 3000000 < 500000)
    {
      gpio_set_level(PIN_BUZZER, 0);
    }
    else if (now % 3000000 < 750000)
    {
      gpio_set_level(PIN_BUZZER, 1);
    }
    else if (now % 3000000 < 1000000)
    {
      gpio_set_level(PIN_BUZZER, 0);
    }
    else if (now % 3000000 < 1250000)
    {
      gpio_set_level(PIN_BUZZER, 1);
    }
    else if (now % 3000000 < 3000000)
    {
      gpio_set_level(PIN_BUZZER, 0);
    }
    // TODO: turn on and off buzer
    // TODO: differnt buzzing patterns for low voltage vs low current
  }
}

void esc_enabled_set(bool enabled)
{
  // TODO: enable and disable ESC
  if (enabled)
  {
    gpio_set_level(PIN_ESC_ENABLE, 1);
  }
  else
  {
    gpio_set_level(PIN_ESC_ENABLE, 0);
  }
}

void motor_control_init(void)
{
  // TODO: Initialize timers and channels for every PWM output.
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = SOC_LEDC_TIMER_BIT_WIDTH,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 4000, // Set output frequency at 4 kHz
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_1,
      .timer_sel = LEDC_TIMER_0,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = PIN_DRIVE_BACK_RIGHT_1,
      .duty = 0, // Set duty to 0%
      .hpoint = 0,
  };

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_2;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_FRONT_RIGHT_2;
  ledc_channel.duty = 0; // Set duty to 0%
  ledc_channel.hpoint = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_3;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_BACK_LEFT_3;
  ledc_channel.duty = 0; // Set duty to 0%
  ledc_channel.hpoint = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_4;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_FRONT_LEFT_4;
  ledc_channel.duty = 0; // Set duty to 0%
  ledc_channel.hpoint = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_5;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_MID_RIGHT_5;
  ledc_channel.duty = 0; // Set duty to 0%
  ledc_channel.hpoint = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_6;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_MID_LEFT_6;
  ledc_channel.duty = 0; // Set duty to 0%
  ledc_channel.hpoint = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void motor_control_set(int16_t left, int16_t right, uint16_t x, uint16_t j2,
                       uint16_t j3)
{
  // TODO: Set PWM duty cycles.
  // scale ledc_set_duty parameter 3 from [INT16_MIN, INT16_MAX] or
  // [0, UINT16_MAX] to [0, (1 << SOC_LEDC_TIMER_BIT_WIDTH) - 1]

  // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
  // ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void current_sense_get(float *esc, float *cell1, float *cell2, float *cell3)
{
  // TODO: Read, calculate, and return ESC and cell sense values.
}
