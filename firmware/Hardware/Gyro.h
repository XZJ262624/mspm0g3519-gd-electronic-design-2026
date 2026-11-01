#ifndef GYRO_H
#define GYRO_H

#include <stdbool.h>
#include <stdint.h>

extern volatile float g_gyro_rate_z_dps;
extern volatile float g_gyro_yaw_deg;
extern volatile uint32_t g_gyro_frame_count;

bool Gyro_Init(void);

float Gyro_GetRateZ(void);
float Gyro_GetYaw(void);

#endif