#include "hardware.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "stdint.h"
#include <esp_log.h>
#include <string.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

bool estop_get(void) {
  if (gpio_get_level(PIN_ESTOP) == 0) {
    return false;
  } else {
    return true;
  }
}

void pins_init() {
  gpio_config_t GPIO_config = {
      .pin_bit_mask = (1ULL << PIN_BUZZER) | (1ULL << PIN_ESC_ENABLE),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };

  gpio_config(&GPIO_config);

  GPIO_config.pin_bit_mask =
      (1ULL << PIN_CURRENT_CELL_1) | (1ULL << PIN_CURRENT_CELL_2) |
      (1ULL << PIN_CURRENT_CELL_3) | (1ULL << PIN_CURRENT_ESC) |
      (1ULL << PIN_ARM_ENCODER_X) | (1ULL << PIN_ARM_ENCODER_J2) |
      (1ULL << PIN_ARM_ENCODER_J3);
  GPIO_config.mode = GPIO_MODE_INPUT;
  GPIO_config.pull_up_en = GPIO_PULLUP_DISABLE;
  GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  GPIO_config.intr_type = GPIO_INTR_DISABLE;

  gpio_config(&GPIO_config);

  GPIO_config.pin_bit_mask = (1ULL << PIN_ESTOP);
  GPIO_config.mode = GPIO_MODE_INPUT;
  GPIO_config.pull_up_en = GPIO_PULLUP_ENABLE;
  GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  GPIO_config.intr_type = GPIO_INTR_DISABLE;

  gpio_config(&GPIO_config);
}

void buzzer_set(bool on) {
  // Set time variable
  int64_t now = esp_timer_get_time();
  // check if buzzer on
  // if buzzer on: turn on
  // check time using module operator to set buzz pattern
  if (on) {
    if (now % 3000000 < 250000) {
      gpio_set_level(PIN_BUZZER, 1);
    } else if (now % 3000000 < 500000) {
      gpio_set_level(PIN_BUZZER, 0);
    } else if (now % 3000000 < 750000) {
      gpio_set_level(PIN_BUZZER, 1);
    } else if (now % 3000000 < 1000000) {
      gpio_set_level(PIN_BUZZER, 0);
    } else if (now % 3000000 < 1250000) {
      gpio_set_level(PIN_BUZZER, 1);
    } else if (now % 3000000 < 3000000) {
      gpio_set_level(PIN_BUZZER, 0);
    }
    // TODO: turn on and off buzer
    // TODO: differnt buzzing patterns for low voltage vs low current
  } else gpio_set_level(PIN_BUZZER, 0);
}

void adc_init(void) {
    // 1 -- Initialize the ADC
   adc_oneshot_unit_init_cfg_t init_config = {
       .unit_id = ADC_UNIT_1,
   };

   ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &cell_sense_handle));

    // 2 -- Configure the ADC
   adc_oneshot_chan_cfg_t config = {
       .bitwidth = ADC_BITWIDTH_13,
       .atten = ADC_ATTEN_DB_12,
   };

   ESP_ERROR_CHECK(adc_oneshot_config_channel(cell_sense_handle,
                                              CELL_SENSE_1_CHANNEL, &config));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(cell_sense_handle,
                                              CELL_SENSE_2_CHANNEL, &config));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(cell_sense_handle,
                                              CELL_SENSE_3_CHANNEL, &config));

    // 3 -- Calibrate the ADC
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_13,
    };
    adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_chan0_handle);

}

void motor_control_init(void) {
  ledc_fade_func_install(0);

  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_RESOLUTION,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = WHEEL_PWM_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel_0 = {
      .gpio_num = PIN_DRIVE_LEFT,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEFT_WHEELS_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_0));

  ledc_channel_config_t ledc_channel_1 = {
      .gpio_num = PIN_DRIVE_RIGHT,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = RIGHT_WHEELS_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_1));
}

void servo_control_init(void) {
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_RESOLUTION,
      .timer_num = LEDC_TIMER_1,
      .freq_hz = SERVO_PWM_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel_2 = {
      .gpio_num = PIN_ARM_PWM_X,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = X_SERVO_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_1,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_2));

  ledc_channel_config_t ledc_channel_3 = {
      .gpio_num = PIN_ARM_PWM_J2,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = J2_SERVO_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_1,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_3));

  ledc_channel_config_t ledc_channel_4 = {
      .gpio_num = PIN_ARM_PWM_J3,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = J3_SERVO_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_1,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_4));
}

#define WHEEL_SPEED 0.5

void set_wheel_speed(int16_t left, int16_t right) {
  left *= WHEEL_SPEED;
  right *= WHEEL_SPEED;

  // convert to 1000 to 2000 micro seconds
  uint16_t left_micro_seconds =
      1000 + (1000 * (((float)((int32_t)left - INT16_MIN)) /
                      (INT16_MAX - INT16_MIN)));
  // convert to percentage of period
  float percentage =
      ((float)left_micro_seconds) / (1000000.0f / (WHEEL_PWM_FREQ_HZ));

  if (percentage > 1.0f) percentage = 1.0f;
  if (percentage < 0.0f) percentage = 0.0f;
  // convert to [0, 2 ** duty_resolution]
  uint32_t duty = ((1 << LEDC_RESOLUTION) - 1) * percentage;
  ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEFT_WHEELS_CHANNEL, duty, 0)); //SUS

  // convert to 1000 to 2000 micro seconds
  uint16_t right_micro_seconds =
      1000 +
      (1000 * (((float)((int32_t)right - INT16_MIN)) / (INT16_MAX - INT16_MIN)));
  // convert to percentage of period
  percentage =
      ((float)right_micro_seconds) / (1000000.0f / (WHEEL_PWM_FREQ_HZ));

  if (percentage > 1.0f) percentage = 1.0f;
  if (percentage < 0.0f) percentage = 0.0f;
  // convert to [0, 2 ** duty_resolution]
  duty = ((1 << LEDC_RESOLUTION) - 1) * percentage;
  ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, RIGHT_WHEELS_CHANNEL, duty, 0));
}

void set_servo_positions(uint16_t x, uint16_t j2, uint16_t j3) {
  // Servo x
  uint16_t micro_seconds;
  float percentage;
  uint32_t duty;
  micro_seconds =
      X_MIN_MICROSECS + x * (X_MAX_MICROSECS - X_MIN_MICROSECS) / (UINT16_MAX); // convert to microseconds(1000-1350)
  percentage =
      ((float)micro_seconds) /
      (1000 * 1000 / (SERVO_PWM_FREQ_HZ)); // convert to percentage of frequency
  duty = ((1 << LEDC_RESOLUTION) - 1) *
         percentage; // convert to [0, 2**duty_resolution]
  ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, X_SERVO_CHANNEL, duty, 0));
  
  // Servo j2
  micro_seconds =
      j2_MIN_MICROSECS + j2 * (j2_MAX_MICROSECS - j2_MIN_MICROSECS) / (UINT16_MAX); // convert to microseconds(700-2300)
  percentage =
      ((float)micro_seconds) /
      (1000 * 1000 / (SERVO_PWM_FREQ_HZ)); // convert to percentage of frequency
  duty = ((1 << LEDC_RESOLUTION) - 1) *
         percentage; // convert to [0, 2**duty_resolution]
  ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, J2_SERVO_CHANNEL, duty, 0));

  // Servo j3
  micro_seconds =
      j3_MIN_MICROSECS + j3 * (j3_MAX_MICROSECS - j3_MIN_MICROSECS) / (UINT16_MAX); // convert to microseconds(700-2300)
  percentage =
      ((float)micro_seconds) /
      (1000 * 1000 / (SERVO_PWM_FREQ_HZ)); // convert to percentage of frequency
  duty = ((1 << LEDC_RESOLUTION) - 1) *
         percentage; // convert to [0, 2**duty_resolution]
  ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, J3_SERVO_CHANNEL, duty, 0));
}

void cell_sense_get(float *cell1, float *cell2, float *cell3) {
   int raw_value_1;
   int raw_value_2;
   int raw_value_3;

   int output_voltage_1;
   int output_voltage_2;
   int output_voltage_3;

   float Vmax = 1.1;
   int16_t Dmax = 1 << ADC_BITWIDTH_13;

   adc_oneshot_read(cell_sense_handle, CELL_SENSE_1_CHANNEL, &raw_value_1);
   adc_oneshot_read(cell_sense_handle, CELL_SENSE_2_CHANNEL, &raw_value_2);
   adc_oneshot_read(cell_sense_handle, CELL_SENSE_3_CHANNEL, &raw_value_3);

    adc_cali_raw_to_voltage(adc1_cali_chan0_handle, raw_value_1, &output_voltage_1);
    adc_cali_raw_to_voltage(adc1_cali_chan0_handle, raw_value_2, &output_voltage_2);
    adc_cali_raw_to_voltage(adc1_cali_chan0_handle, raw_value_3, &output_voltage_3);

   *cell1 = CELL1_VOLTAGE_MAX * output_voltage_1 / Vmax;
   *cell2 = CELL2_VOLTAGE_MAX * output_voltage_2 / Vmax;
   *cell3 = CELL3_VOLTAGE_MAX * output_voltage_3 / Vmax;
  }