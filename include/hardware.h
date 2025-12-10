#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdbool.h>
#include <stdint.h>

// TODO: Update from schematic
#define PIN_ESTOP 19
#define PIN_BUZZER 0
#define PIN_ESC_ENABLE 37
#define PIN_DRIVE_FRONT_RIGHT_2 34
#define PIN_DRIVE_FRONT_LEFT_4 26
#define PIN_DRIVE_MID_RIGHT_5 21
#define PIN_DRIVE_MID_LEFT_6 20
#define PIN_DRIVE_BACK_RIGHT_1 35
#define PIN_DRIVE_BACK_LEFT_3 33
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

bool estop_get(void);

void buzzer_set(bool on);

void pins_init();

void esc_enabled_set(bool enabled);

void motor_control_init(void);
void motor_control_set(int16_t left, int16_t right, uint16_t x, uint16_t j2,
                       uint16_t j3);

void current_sense_get(float *esc, float *cell1, float *cell2, float *cell3);

#endif
