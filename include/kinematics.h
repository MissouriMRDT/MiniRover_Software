#ifndef _KINEMATICS_H_
#define _KINEMATICS_H_

#include "math.h"
#include <stdint.h>

#define L1 13.5 // length from joint 1 to joint 2
#define L2 25 // length from joint 2 to center of gripper
#define MAX_REACH (L1 + L2) // max reach
#define MIN_REACH (L2 - L1) // min reach
#define OFFSET 0 // Offset to put zero degrees at y axis

void fk_calculate_position(uint16_t j2, uint16_t j3, int16_t *y,
                           int16_t *z);

void ik_calculate_angles(int16_t y, int16_t z,
                         uint16_t *j2, uint16_t *j3);

#endif
