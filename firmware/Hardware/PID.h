#ifndef PID_H
#define PID_H

#include <stdbool.h>

typedef struct
{
    float kp;
    float ki;
    float kd;

    float sample_time_s;

    float output_min;
    float output_max;

    float integrator_min;
    float integrator_max;

    float integrator;
    float previous_error;
    float previous_measurement;
    float differentiator;

    float d_alpha;
    float d_beta;

    bool initialized;
} PID_Controller_t;

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
    float integrator_max);

void PID_Reset(PID_Controller_t *pid);

float PID_Update(
    PID_Controller_t *pid,
    float target,
    float measurement);

#endif