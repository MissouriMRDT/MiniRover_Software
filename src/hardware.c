#include "hardware.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "stdint.h"
#include "esp_timer.h"
#include "vesc.h"
#include "driver/uart.h"
#include <string.h>
#include <esp_log.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

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
      .pin_bit_mask = (1ULL << PIN_BUZZER),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
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

// void set_pwm(ledc_channel_t channel, uint16_t decipercent)
// {
//   // Map uint16 [0, uint16_MAX] to duty cycle [0, 2**resolution]
//   uint32_t duty;
//   duty = (decipercent * (1 << SOC_LEDC_TIMER_BIT_WIDTH)) / (UINT16_MAX);
//   ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
//   // take in channel and duty cycle(0-1000 deci%)
//   // convert duty cycle to -bit(-resolution-same as ledc-timer resolution)
//   ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));

//   ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
//   // update
//   ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));
// }

// void set_pulse_width(ledc_channel_t channel, int16_t pulse_width)
// {
//   // Map decipercent [0, 1000] to duty cycle [0, 2**resolution]
//   uint32_t duty;
//   duty = 1000 + (1000 * (pulse_width - INT16_MIN) / (INT16_MAX - INT16_MIN));
//   // duty = (pulse_width * (1 << 8)) / 1000;
//   ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
//   // take in channel and duty cycle(0-1000 deci%)
//   // convert duty cycle to -bit(-resolution-same as ledc-timer resolution)
//   ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));

//   ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
//   // update
//   ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));
// }

// void set_fade(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t micro_seconds, int desired_fade_time_ms)
// {

//   // convert to percentage of period
//   uint32_t freq_hz = 50;
//   float percentage = (micro_seconds - 1000) / (1000 * 1000 / (freq_hz));

//   // convert to [0, 2 ** duty_resolution]
//   uint32_t duty;
//   duty = (1 << SOC_LEDC_TIMER_BIT_WIDTH) * percentage;

//   ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, channel, duty, desired_fade_time_ms);
//   ledc_fade_start(LEDC_LOW_SPEED_MODE, channel, LEDC_FADE_NO_WAIT);
// }

void adc_init(void)
{
  adc_oneshot_unit_handle_t cell_sense_handle;
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &cell_sense_handle));

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12,
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(cell_sense_handle, CELL_SENSE_1_CHNL, &config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(cell_sense_handle, CELL_SENSE_2_CHNL, &config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(cell_sense_handle, CELL_SENSE_3_CHNL, &config));

  // Find the raw value to then convert later int raw_value_1;
  int raw_value_1;
  int raw_value_2;
  int raw_value_3;

  adc_oneshot_read(cell_sense_handle, CELL_SENSE_1_CHNL, &raw_value_1);
  printf("ADC Raw Value: %d\n", raw_value_1);

  adc_oneshot_read(cell_sense_handle, CELL_SENSE_2_CHNL, &raw_value_2);
  printf("ADC Raw Value: %d\n", raw_value_2);

  adc_oneshot_read(cell_sense_handle, CELL_SENSE_3_CHNL, &raw_value_3);
  printf("ADC Raw Value: %d\n", raw_value_3);

  // int converted_result;
  // ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(cell_sense_handle, NULL, CELL_SENSE_1_CHNL, &converted_result));
  // ESP_LOGI("hardware.c", "cell_sense_1", converted_result);

  // ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(cell_sense_handle, NULL, CELL_SENSE_2_CHNL, &converted_result));
  // ESP_LOGI("hardware.c", "cell_sense_2", converted_result);

  // ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(cell_sense_handle, NULL, CELL_SENSE_3_CHNL, &converted_result));
  // ESP_LOGI("hardware.c", "cell_sense_3", converted_result);

  // Convert to a voltage
  // Vout = Dout * Vmax / Dmax
  // Where Dout is the raw ADC value, Vmax is the maximum input voltage, and Dmax is 2^bitwidth.

  // Find Cell voltage bitwidth

  // Maximum of the output ADC raw digital reading result, which is 2^bitwidth,
  // where bitwidth is the adc_oneshot_chan_cfg_t::bitwidth configured before.
}

void motor_control_init(void)
{
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = SOC_LEDC_TIMER_BIT_WIDTH,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = WHEEL_PWM_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
  ledc_channel_config_t ledc_channel = {
      .gpio_num = PIN_DRIVE_RIGHT_1,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_0;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_RIGHT_2;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_0;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_RIGHT_3;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_1;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_LEFT_1;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_1;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_LEFT_2;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_1;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_DRIVE_LEFT_3;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void servo_control_init(void)
{
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = SOC_LEDC_TIMER_BIT_WIDTH,
      .timer_num = LEDC_TIMER_1,
      .freq_hz = SERVO_PWM_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
  ledc_channel_config_t ledc_channel = {
      .gpio_num = PIN_ARM_PWM_X,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_2,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_1,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_3;
  ledc_channel.timer_sel = LEDC_TIMER_1;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_ARM_PWM_J2;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_4;
  ledc_channel.timer_sel = LEDC_TIMER_1;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = PIN_ARM_PWM_J3;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void set_wheel_speed(int16_t left, int16_t right)
{
  // left set fading
  uint16_t left_micro_seconds;
  // convert to 1000 to 2000 micro seconds
  left_micro_seconds = 1000 + (1000 * (((float)((int32_t)left - INT16_MIN)) / (INT16_MAX - INT16_MIN)));
  // convert to percentage of period
  float percentage = ((float)left_micro_seconds) / (1000 * 1000 / (WHEEL_PWM_FREQ_HZ));
  // convert to [0, 2 ** duty_resolution]
  uint32_t duty;
  duty = (1 << SOC_LEDC_TIMER_BIT_WIDTH) * percentage;
  ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEFT_WHEELS_CHNL, duty, WHEEL_FADE_TIME);
  ledc_fade_start(LEDC_LOW_SPEED_MODE, LEFT_WHEELS_CHNL, LEDC_FADE_NO_WAIT);

  // right set fading
  uint16_t right_micro_seconds;
  // convert to 1000 to 2000 micro seconds
  right_micro_seconds = 1000 + (1000 * (((float)((int32_t)right - INT16_MIN)) / (INT16_MAX - INT16_MIN)));
  // convert to percentage of period
  percentage = ((float)right_micro_seconds) / (1000 * 1000 / (WHEEL_PWM_FREQ_HZ));
  // convert to [0, 2 ** duty_resolution]
  duty = (1 << SOC_LEDC_TIMER_BIT_WIDTH) * percentage;
  ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, RIGHT_WHEELS_CHNL, duty, WHEEL_FADE_TIME);
  ledc_fade_start(LEDC_LOW_SPEED_MODE, RIGHT_WHEELS_CHNL, LEDC_FADE_NO_WAIT);

  // set_fade(LEDC_LOW_SPEED_MODE, LEFT_WHEELS_CHNL, left_micro_seconds, 1000);
  // set_fade(LEDC_LOW_SPEED_MODE, RIGHT_WHEELS_CHNL, right, 1000);
}

void set_servo_positions(uint16_t x, uint16_t j2, uint16_t j3)
{
  // Servo x
  uint16_t micro_seconds;
  float percentage;
  uint32_t duty;
  micro_seconds = 500 + x * 2000 / (UINT16_MAX);                             // convert to microseconds(500-2500)
  percentage = ((float)micro_seconds) / (1000 * 1000 / (SERVO_PWM_FREQ_HZ)); // convert to percentage of frequency
  duty = (1 << SOC_LEDC_TIMER_BIT_WIDTH) * percentage;                       // convert to [0, 2**duty_resolution]
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, X_SERVO_CHNL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, X_SERVO_CHNL));

  ESP_LOGI("hardwre.c", "micro_seconds: %d", micro_seconds);
  ESP_LOGI("hardwre.c", "percentage: %f", percentage);
  ESP_LOGI("hardwre.c", "duty: %d", duty);
  int get_duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, X_SERVO_CHNL);
  ESP_LOGI("hardware.c", "get_duty: %d", get_duty);

  // Servo j2
  micro_seconds = 500 + j2 * 2000 / (UINT16_MAX);                            // convert to microseconds(500-2500)
  percentage = ((float)micro_seconds) / (1000 * 1000 / (SERVO_PWM_FREQ_HZ)); // convert to percentage of frequency
  duty = (1 << SOC_LEDC_TIMER_BIT_WIDTH) * percentage;                       // convert to [0, 2**duty_resolution]
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, J2_SERVO_CHNL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, J2_SERVO_CHNL));

  // Servo j3
  micro_seconds = 500 + j3 * 2000 / (UINT16_MAX);                            // convert to microseconds(500-2500)
  percentage = ((float)micro_seconds) / (1000 * 1000 / (SERVO_PWM_FREQ_HZ)); // convert to percentage of frequency
  duty = (1 << SOC_LEDC_TIMER_BIT_WIDTH) * percentage;                       // convert to [0, 2**duty_resolution]
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, J3_SERVO_CHNL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, J3_SERVO_CHNL));

  // set_pwm(X_SERVO_CHNL, x);
  // set_pwm(J2_SERVO_CHNL, j2);
  // set_pwm(J3_SERVO_CHNL, j3);
}

// void set_wheel_speed_uart(int16_t left, int16_t right) // change from esc to vesc
// {
//   float float_left = (float)left / INT16_MAX;
//   float float_right = (float)right / INT16_MAX;
//   vesc_drive(float_left, UART_NUM_1);
//   vesc_drive(float_right, UART_NUM_MAX);
//   ESP_LOGI("hardware.c", "left speed %.2f", float_left);
//   ESP_LOGI("hardware.c", "right speed %.2f", float_right);
// }

void cell_sense_get(float *cell1, float *cell2, float *cell3)
{

  // TODO: Read, calculate, and return ESC and cell sense values.
}

// void uart_init(int uart_num)
// {
//   ESP_ERROR_CHECK(uart_driver_install(uart_num, 2048, 2048, 0, NULL, 0));
//   uart_config_t uart_config = {
//       .baud_rate = 115200,
//       .data_bits = UART_DATA_8_BITS,
//       .parity = UART_PARITY_DISABLE,
//       .stop_bits = UART_STOP_BITS_1,
//       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//   };
//   // Configure UART parameters
//   ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
//   ESP_ERROR_CHECK(uart_set_pin(uart_num, 35, 34, 33, 26));
// }

// void vesc_drive(float duty, int uart_num)
// {
//   uint8_t payload[5];
//   payload[0] = COMM_SET_DUTY;
//   int32_t number = duty * 100000;
//   payload[1] = number >> 24;
//   payload[2] = number >> 16;
//   payload[3] = number >> 8;
//   payload[4] = number;

//   uint16_t crcPayload = crc16(payload, 5);
//   int count = 0;
//   uint8_t messageSend[256];
//   messageSend[count++] = 2;
//   messageSend[count++] = 5;
//   memcpy(messageSend + count, payload, 5);
//   count += 5;

//   messageSend[count++] = (uint8_t)(crcPayload >> 8);
//   messageSend[count++] = (uint8_t)(crcPayload & 0xFF);
//   messageSend[count++] = 3;

//   uart_write_bytes(uart_num, messageSend, 256);
// }