#include "kinematics.h"
#include "esp_log.h"

void fk_calculate_position(uint16_t j2, uint16_t j3, int16_t *y,
                           int16_t *z) {
  float j2f = ((j2 * 2 * M_PI) / UINT16_MAX);
  float j3f = ((j3 * 2 * M_PI) / UINT16_MAX);
  float radia = sqrtf((L1*L1) + (L2*L2) - (2*L1*L2*cosf(j3f)));
  float theta =  j2f - (acosf((radia*radia + L1*L1 - L2*L2) / (2*radia*L1)));
  float yf = (radia * cosf(theta)) / MAX_REACH;
  float zf = (radia * sinf(theta)) / MAX_REACH;
  *y = 500 * yf; // for -500 to 500 rather than -maxreach to maxreach
  *z = 500 * zf;
                            // TODO: set y, z based on FK from j2, j3
}

void ik_calculate_angles(int16_t y, int16_t z,
                         uint16_t *j2, uint16_t *j3) {
  float yf = (y * MAX_REACH) / 500;
  float zf = (z * MAX_REACH) / 500;
  float radia = sqrtf(yf*yf + zf*zf);
  if (radia >= MIN_REACH && radia <= MAX_REACH) {
    float theta = atan2f(zf, yf);
    float angle_j3 = acosf((L1*L1 + L2*L2 - radia*radia) / (2*L1*L2));
    float angle_j2 = acosf((radia*radia + L1*L1 - L2*L2) / (2*radia*L1)) + theta + OFFSET;
                            // TODO: set j2, j3 based on IK from y, z
    *j3 = (angle_j3) * UINT16_MAX / (2 * M_PI);
    *j2 = (angle_j2) * UINT16_MAX / (2 * M_PI);
  }
}
