#ifndef __RAMP_H__
#define __RAMP_H__

// 速度步长
#define ACC_SPEED_STEP 12.0f // 加速步长
#define DEC_SPEED_STEP 12.0f // 减速步长

typedef struct
{
    float last_speed;   // 上一次的输出值
    float accel_step; // 升速步长（加速度控制）
    float decel_step; // 降速步长（通常降速可以设得比升速快一些，保证安全）
} Speed_Ramp_t;

float Delta_Target_Speed_Control(Speed_Ramp_t *ramp, float target);

#endif