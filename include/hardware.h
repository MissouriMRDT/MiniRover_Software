#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdbool.h>
#include <stdint.h>
#include "driver/ledc.h"

// TODO: Update from schematic
#define PIN_ESTOP 19
#define PIN_BUZZER 0
#define PIN_ESC_ENABLE 37
#define PIN_DRIVE_RIGHT_1 35
#define PIN_DRIVE_RIGHT_2 34
#define PIN_DRIVE_RIGHT_3 33
#define PIN_DRIVE_LEFT_1 26
#define PIN_DRIVE_LEFT_2 21
#define PIN_DRIVE_LEFT_3 20
#define PIN_ARM_PWM_X 6
#define PIN_ARM_PWM_J2 8
#define PIN_ARM_PWM_J3 10
#define PIN_ARM_ENCODER_X 5
#define PIN_ARM_ENCODER_J2 7
#define PIN_ARM_ENCODER_J3 9
#define PIN_CURRENT_ESC 4
#define PIN_CURRENT_CELL_1 1
#define PIN_CURRENT_CELL_2 2
#define PIN_CURRENT_CELL_3 3

#define LEFT_WHEELS_CHNL LEDC_CHANNEL_0
#define RIGHT_WHEELS_CHNL LEDC_CHANNEL_1
#define X_SERVO_CHNL LEDC_CHANNEL_2
#define J2_SERVO_CHNL LEDC_CHANNEL_3
#define J3_SERVO_CHNL LEDC_CHANNEL_4

bool estop_get(void);

void buzzer_set(bool on);

void pins_init();

void set_pwm(ledc_channel_t channel, uint16_t duty_cycle);
void set_pulse_width(ledc_channel_t channel, int16_t pulse_width);

void set_fade(ledc_mode_t speed_mode, ledc_channel_t channel, int16_t target_duty, int desired_fade_time_ms);

void esc_enabled_set(bool enabled);

void motor_control_init(void);
void set_wheel_speed(int16_t left, int16_t right);

void servo_control_init(void);
void set_servo_positions(uint16_t x, uint16_t j2, uint16_t j3);

// void adc_oneshot_init(void);

void cell_sense_get(float *cell1, float *cell2, float *cell3);

// void set_wheel_speed_uart(int16_t left, int16_t right);
// void uart_init(int uart_num);
// void vesc_drive(float duty, int uart_num);

#endif
