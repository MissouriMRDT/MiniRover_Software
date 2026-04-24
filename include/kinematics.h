#ifndef _KINEMATICS_H_
#define _KINEMATICS_H_

#include <stdint.h>

void fk_calculate_position(uint16_t j2, uint16_t j3, int16_t *x,
                           int16_t *y);

void ik_calculate_angles(int16_t x, int16_t y,
                         uint16_t *j2, uint16_t *j3);

#endif
