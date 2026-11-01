#include "PID.h"

static float PID_Clamp(
    float value,
    float minimum,
    float maximum)
{
    if (value > maximum)
    {
        return maximum;
    }

    if (value < minimum)
    {
        return minimum;
    }

    return value;
}

void PID_Init(
    PID_Controller_t *pid,
    float kp,
    float ki,
    float kd,
    float sample_time_s,
    float derivative_filter_time_s,
    float output_min,
    float output_max,
    float integrator_min,
    float integrator_max)
{
    float denominator;

    if (pid == 0)
    {
        return;
    }

    if (sample_time_s <= 0.0f)
    {
        sample_time_s = 0.01f;
    }

    if (derivative_filter_time_s <
        (0.5f * sample_time_s))
    {
        derivative_filter_time_s =
            0.5f * sample_time_s;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->sample_time_s = sample_time_s;

    pid->output_min = output_min;
    pid->output_max = output_max;

    pid->integrator_min = integrator_min;
    pid->integrator_max = integrator_max;

    denominator =
        2.0f * derivative_filter_time_s +
        sample_time_s;

    if (kd > 0.0f)
    {
        pid->d_alpha =
            (2.0f * derivative_filter_time_s -
             sample_time_s) /
            denominator;

        pid->d_beta =
            (2.0f * kd) /
            denominator;
    }
    else
    {
        pid->d_alpha = 0.0f;
        pid->d_beta = 0.0f;
    }

    PID_Reset(pid);
}

void PID_Reset(PID_Controller_t *pid)
{
    if (pid == 0)
    {
        return;
    }

    pid->integrator = 0.0f;
    pid->previous_error = 0.0f;
    pid->previous_measurement = 0.0f;
    pid->differentiator = 0.0f;
    pid->initialized = false;
}

float PID_Update(
    PID_Controller_t *pid,
    float target,
    float measurement)
{
    float error;
    float proportional;
    float integrator_candidate;
    float unsaturated_output;
    float output;
    bool allow_integration;

    if (pid == 0)
    {
        return 0.0f;
    }

    error = target - measurement;

    if (!pid->initialized)
    {
        pid->previous_error = error;
        pid->previous_measurement = measurement;
        pid->differentiator = 0.0f;
        pid->initialized = true;
    }

    proportional = pid->kp * error;

    integrator_candidate =
        pid->integrator +
        0.5f *
        pid->ki *
        pid->sample_time_s *
        (error + pid->previous_error);
        
    integrator_candidate =
        PID_Clamp(
            integrator_candidate,
            pid->integrator_min,
            pid->integrator_max);

    if (pid->kd > 0.0f)
    {
        pid->differentiator =
            pid->d_alpha *
            pid->differentiator -
            pid->d_beta *
            (measurement -
             pid->previous_measurement);
    }
    else
    {
        pid->differentiator = 0.0f;
    }

    unsaturated_output =
        proportional +
        integrator_candidate +
        pid->differentiator;

    allow_integration = true;

    if ((unsaturated_output > pid->output_max) &&
        (error > 0.0f))
    {
        allow_integration = false;
    }

    if ((unsaturated_output < pid->output_min) &&
        (error < 0.0f))
    {
        allow_integration = false;
    }

    if (allow_integration)
    {
        pid->integrator = integrator_candidate;
    }

    output =
        proportional +
        pid->integrator +
        pid->differentiator;

    output =
        PID_Clamp(
            output,
            pid->output_min,
            pid->output_max);

    pid->previous_error = error;
    pid->previous_measurement = measurement;
    
    return output;
}