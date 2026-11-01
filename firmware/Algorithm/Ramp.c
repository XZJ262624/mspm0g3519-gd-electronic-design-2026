#include "Ramp.h"
#include "math.h"

/**
 * @brief 目标速度斜坡控制函数
 * @param ramp 速度斜坡控制结构体指针
 * @param target 目标速度 (m/s)
 * @return 当前输出速度 (m/s)
 */
float Delta_Target_Speed_Control(Speed_Ramp_t *ramp, float target)
{
    float error = target - ramp->last_speed;
    float step = 0;

    // 根据目标速度与当前输出的关系，判断是加速还是减速，加速步长小于减速步长
    float abs_target = fabs(target);
    float abs_last = fabs(ramp->last_speed);
    step = (target * ramp->last_speed < 0) ? ramp->decel_step : ((abs_target > abs_last) ? ramp->accel_step : ramp->decel_step);
    // 计算出步进后的速度
    ramp->last_speed = (error > step) ? ramp->last_speed + step : ((error < -step) ? ramp->last_speed - step : target);

    return ramp->last_speed;
}