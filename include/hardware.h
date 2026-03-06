#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include <stdbool.h>
#include <stdint.h>

// TODO: Update from schematic
#define PIN_ESTOP 19
#define PIN_BUZZER 11
#define PIN_ESC_ENABLE 37
#define PIN_DRIVE_RIGHT 35
#define PIN_DRIVE_LEFT 26
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

#define LEFT_WHEELS_CHANNEL LEDC_CHANNEL_0
#define RIGHT_WHEELS_CHANNEL LEDC_CHANNEL_1
#define X_SERVO_CHANNEL LEDC_CHANNEL_2
#define J2_SERVO_CHANNEL LEDC_CHANNEL_3
#define J3_SERVO_CHANNEL LEDC_CHANNEL_4
#define CELL_SENSE_1_CHANNEL ADC_CHANNEL_0
#define CELL_SENSE_2_CHANNEL ADC_CHANNEL_1
#define CELL_SENSE_3_CHANNEL ADC_CHANNEL_2
#define WHEEL_PWM_FREQ_HZ 50
#define SERVO_PWM_FREQ_HZ 50
#define LEDC_RESOLUTION 8
#define WHEEL_FADE_TIME 10000
#define X_MIN_MICROSECS 1000
#define X_MAX_MICROSECS 1350
#define j2_MIN_MICROSECS 700
#define j2_MAX_MICROSECS 2000
#define j3_MIN_MICROSECS 700
#define j3_MAX_MICROSECS 2300



bool estop_get(void);

void buzzer_set(bool on);

void pins_init();

void esc_enabled_set(bool enabled);

void motor_control_init(void);
void set_wheel_speed(int16_t left, int16_t right);

void servo_control_init(void);
void set_servo_positions(uint16_t x, uint16_t j2, uint16_t j3);

void adc_init(void);

void cell_sense_get(float *cell1, float *cell2, float *cell3);

#endif
