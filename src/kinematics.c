#include "kinematics.h"

void fk_calculate_position(uint16_t ax, uint16_t j2, uint16_t j3, int16_t *x,
                           int16_t *y, int16_t *z) {
  // TODO: set x, y, z based on FK from ax, j2, j3
}

void ik_calculate_angles(int16_t x, int16_t y, int16_t z, uint16_t *ax,
                         uint16_t *j2, uint16_t *j3) {
  // TODO: set ax, j2, j3 based on IK from x, y, z
}
