/**
  ****************************************************************************
  * @file    Motor.c
  * @author  XZJxsdss
  * @date    2025_7_30
  * @brief   电机测速及PID控制
  * 2025_8_2 update_stm32_STM_PID_1(2025_7_25)移植定时中断测速部分
  ****************************************************************************
**/

/**
  ****************************************************************************
  * @date    2026_8_12
  * @brief   重构
  ****************************************************************************
**/

#include "Board_Config.h"
#include "Encoder.h"
#include "Motor.h"
#include "PWM.h"
#include "PID.h"
#include "Ramp.h"

#define SPEED_CONTROL_PERIOD_S    0.010f

#define SPEED_PID_KP              0.165f
#define SPEED_PID_KI              0.085f
#define SPEED_PID_KD              0.001f

#define SPEED_PID_D_FILTER_S      0.020f

#define CHASSIS_START_RAMP_STEP    (0.005f)

//巡线启动ramp
static Speed_Ramp_t chassis_start_ramp = {
    .last_speed = 0.0f,
    .accel_step = CHASSIS_START_RAMP_STEP,//perms
    .decel_step = 1.5f,
};

Speed_Ramp_t chassis_left_speed_ramp = {
    .last_speed = 0.0f,
    .accel_step = ACC_SPEED_STEP,
    .decel_step = DEC_SPEED_STEP,
};

Speed_Ramp_t chassis_right_speed_ramp = {
    .last_speed = 0.0f,
    .accel_step = ACC_SPEED_STEP,
    .decel_step = DEC_SPEED_STEP,
};

static PID_Controller_t left_pid;//leftPID
static PID_Controller_t right_pid;//rightPID

static volatile int32_t left_target_count;//左轮目标速度
static volatile int32_t right_target_count;//右轮目标速度

static volatile bool motor_enabled;//当前要求的电机使能状态
static bool previoumotor_enabled;//速度控制器上一次处理时的使能状态

static bool chassis_start_ramp_enabled = false;

void MotorControl_SetStartRampEnabled(bool enabled)
{
    chassis_start_ramp_enabled = enabled;

    /*
     * 每次切换Ramp状态都从0重新开始，
     * 避免继承上一个任务的启动进度。
     */
    chassis_start_ramp.last_speed = 0.0f;
}

//    当前使能               上次状态 	              含义	          动作
// motor_enabled	 previoumotor_enabled
//       0	                   0	 	            持续停止	   电机输出0
//       0	                   1		            刚停止	       清目标、复位PID、停止电机
//       1	                   0		            刚启动	       复位PID，然后开始闭环
//       1	                   1		            持续运行	   正常执行PID
// MotorControl_SetEnabled(true);   // 启动闭环
// MotorControl_SetEnabled(false);  // 停止闭环

static int16_t Motor_FloatToCommand(float value)
{
    if (value > 100.0f)
    {
        value = 100.0f;
    }
    else if (value < -100.0f)
    {
        value = -100.0f;
    }

    if (value >= 0.0f)
    {
        return (int16_t)(value + 0.5f);
    }

    return (int16_t)(value - 0.5f);
}

//初始化整个双轮速度闭环控制模块，必须在速度控制定时器TimG0启动前调用
void MotorControl_Init(void)
{
    left_target_count = 0;
    right_target_count = 0;

    chassis_start_ramp.last_speed = 0.0f;

    motor_enabled = false;
    previoumotor_enabled = false;

    PID_Init(
        &left_pid,
        SPEED_PID_KP,
        SPEED_PID_KI,
        SPEED_PID_KD,
        SPEED_CONTROL_PERIOD_S,
        SPEED_PID_D_FILTER_S,
        -100.0f,
        100.0f,
        -80.0f,
        80.0f);

    PID_Init(
        &right_pid,
        SPEED_PID_KP,
        SPEED_PID_KI,
        SPEED_PID_KD,
        SPEED_CONTROL_PERIOD_S,
        SPEED_PID_D_FILTER_S,
        -100.0f,
        100.0f,
        -80.0f,
        80.0f);

    MotorDriver_SetLeftCommand(0);
    MotorDriver_SetRightCommand(0);
}

//设置左轮目标速度，单位：编码器计数/10ms
void MotorControl_SetLeftTarget(
    int32_t count_per_10ms)
{
    left_target_count = count_per_10ms;
}
//设置右轮目标速度，单位：编码器计数/10ms
void MotorControl_SetRightTarget(
    int32_t count_per_10ms)
{
    right_target_count = count_per_10ms;
}

void MotorControl_SetLeftRightTarget(
    int32_t count_per_10ms_L,
    int32_t count_per_10ms_R)
{
    float start_scale;
    int32_t target_left_ramp;
    int32_t target_right_ramp;

    /*
     * task2关闭Ramp后，目标速度直接送入电机闭环。
     */
    if (!chassis_start_ramp_enabled)
    {
        MotorControl_SetLeftTarget(
            count_per_10ms_L);

        MotorControl_SetRightTarget(
            count_per_10ms_R);

        return;
    }

    /*
     * 停车立即执行，同时复位Ramp。
     */
    if ((count_per_10ms_L == 0) &&
        (count_per_10ms_R == 0))
    {
        chassis_start_ramp.last_speed = 0.0f;

        MotorControl_SetLeftTarget(0);
        MotorControl_SetRightTarget(0);
        return;
    }

    start_scale =
        Delta_Target_Speed_Control(
            &chassis_start_ramp,
            1.0f);

    target_left_ramp =
        (int32_t)(
            (float)count_per_10ms_L *
            start_scale);

    target_right_ramp =
        (int32_t)(
            (float)count_per_10ms_R *
            start_scale);

    MotorControl_SetLeftTarget(
        target_left_ramp);

    MotorControl_SetRightTarget(
        target_right_ramp);
}

void MotorControl_GetTargets(
    int32_t *left_target,
    int32_t *right_target)
{
    uint32_t previous_primask;

    previous_primask = __get_PRIMASK();
    __disable_irq();

    if (left_target != 0)
    {
        *left_target = left_target_count;
    }

    if (right_target != 0)
    {
        *right_target = right_target_count;
    }

    if ((previous_primask & 1U) == 0U)
    {
        __enable_irq();
    }
}

void MotorControl_SetEnabled(bool enabled)
{
    if (enabled)
    {
        /*
         * 每次从停止切换到运行时，
         * 重新从0开始启动Ramp。
         */
        if (!motor_enabled)
        {
            chassis_start_ramp.last_speed = 0.0f;
        }
    }
    else
    {
        /*
         * 停车仍然立即执行，不经过缓慢减速Ramp。
         */
        left_target_count = 0;
        right_target_count = 0;

        chassis_start_ramp.last_speed = 0.0f;
    }

    motor_enabled = enabled;
}

void MotorControl_ToggleEnabled(void)//使能状态反转
{
    motor_enabled = !motor_enabled;
}
bool MotorControl_IsEnabled(void)
{
    return motor_enabled;//读取当前使能状态
}

//每10ms速度闭环更新
void MotorControl_Update10ms(void)
{
    EncoderSample_t encoder_sample;

    float left_output;
    float right_output;

    Encoder_ReadDelta(&encoder_sample);//读取左右编码器本周期计数

    //判断电机是否使能
    if (!motor_enabled)//如果是刚从运行切换到停止
    {
        if (previoumotor_enabled)
        {
            left_target_count = 0;//如果是从启动到停止，将target_count清零
            right_target_count = 0;//如果是从启动到停止，将target_count清零

            PID_Reset(&left_pid);//pid复位
            PID_Reset(&right_pid);
        }

        previoumotor_enabled = false;//更新状态

        MotorDriver_SetLeftCommand(0);//左轮停车
        MotorDriver_SetRightCommand(0);//右轮停车
        return;
    }
    
    if (!previoumotor_enabled)//如果是刚从停止切换到运行，不需要将target_count清零,在后续
    {
        PID_Reset(&left_pid);//pid复位
        PID_Reset(&right_pid);

        previoumotor_enabled = true;//更新状态
    }

    left_output =
        PID_Update(
            &left_pid,
            (float)left_target_count,
            (float)encoder_sample.left_count);

    right_output =
        PID_Update(
            &right_pid,
            (float)right_target_count,
            (float)encoder_sample.right_count);

    MotorDriver_SetLeftCommand(
        Motor_FloatToCommand(left_output));

    MotorDriver_SetRightCommand(
        Motor_FloatToCommand(right_output));
}

static int16_t Motor_ClampCommand(int16_t command)//将command限制在-100到100
{
    if (command > 100)
    {
        return 100;
    }

    if (command < -100)
    {
        return -100;
    }

    return command;
}

static uint16_t Motor_GetCommandMagnitude(int16_t command)//取command绝对值
{
    if (command < 0)
    {
        return (uint16_t)(-command);
    }

    return (uint16_t)command;
}

// 左轮底层电机命令
// command > 0：IN1输出PWM，IN2保持低电平
// command < 0：IN1保持低电平，IN2输出PWM
// command = 0：IN1、IN2均为低电平，滑行停止
void MotorDriver_SetLeftCommand(int16_t command)
{
    uint16_t duty;
    uint16_t pwm_duty;

    command = -Motor_ClampCommand(command);

    if (command > 0)
    {
        duty = Motor_GetCommandMagnitude(command);

        pwm_duty = (uint16_t)(100U - duty);

        PWM_SetLeftIN1Duty(100U);
        PWM_SetLeftIN2Duty(pwm_duty);
    }
    else if (command < 0)
    {
        duty = Motor_GetCommandMagnitude(command);

        pwm_duty = (uint16_t)(100U - duty);

        PWM_SetLeftIN1Duty(pwm_duty);
        PWM_SetLeftIN2Duty(100U);
    }
    else
    {
        PWM_SetLeftIN1Duty(0U);
        PWM_SetLeftIN2Duty(0U);
    }
}

// 右轮底层电机命令
// command > 0：IN1输出PWM，IN2保持低电平
// command < 0：IN1保持低电平，IN2输出PWM
// command = 0：IN1、IN2均为低电平，滑行停止
void MotorDriver_SetRightCommand(int16_t command)
{
    uint16_t duty;
    uint16_t pwm_duty;

    command = -Motor_ClampCommand(command);

    if (command > 0)
    {
        duty = Motor_GetCommandMagnitude(command);
        pwm_duty = (uint16_t)(100U - duty);

        PWM_SetRightIN1Duty(100U);
        PWM_SetRightIN2Duty(pwm_duty);
    }
    else if (command < 0)
    {
        duty = Motor_GetCommandMagnitude(command);
        pwm_duty = (uint16_t)(100U - duty);

        PWM_SetRightIN1Duty(pwm_duty);
        PWM_SetRightIN2Duty(100U);
    }
    else
    {
        PWM_SetRightIN1Duty(0U);
        PWM_SetRightIN2Duty(0U);
    }
}