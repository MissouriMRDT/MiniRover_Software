#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdbool.h>
#include <stdint.h>

// TODO: Update from schematic
#define PIN_ESTOP 0
#define PIN_BUZZER 0
#define PIN_ESC_ENABLE 0
#define PIN_DRIVE_FRONT_RIGHT 0
#define PIN_DRIVE_FRONT_LEFT 0
#define PIN_DRIVE_MID_RIGHT 0
#define PIN_DRIVE_MID_LEFT 0
#define PIN_DRIVE_BACK_RIGHT 0
#define PIN_DRIVE_BACK_LEFT 0
#define PIN_ARM_PWM_X 0
#define PIN_ARM_PWM_J2 0
#define PIN_ARM_PWM_J3 0
#define PIN_ARM_ENCODER_X 0
#define PIN_ARM_ENCODER_J2 0
#define PIN_ARM_ENCODER_J3 0
#define PIN_CURRENT_ESC 0
#define PIN_CURRENT_CELL_1 0
#define PIN_CURRENT_CELL_2 0
#define PIN_CURRENT_CELL_3 0

bool estop_get(void);

void buzzer_set(bool on);

void esc_enabled_set(bool enabled);

void motor_control_init(void);
void motor_control_set(int16_t left, int16_t right, uint16_t x, uint16_t j2,
                       uint16_t j3);

void current_sense_get(float *esc, float *cell1, float *cell2, float *cell3);

#endif
