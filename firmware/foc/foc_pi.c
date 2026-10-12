#include "foc_pi.h"

void foc_pi_init(foc_pi_t *pi, int32_t kp_q10, int32_t ki_q10, q15_t out_min, q15_t out_max)
{
    pi->kp = kp_q10;
    pi->ki = ki_q10;
    pi->integ = 0;
    pi->out_min = out_min;
    pi->out_max = out_max;
}

void foc_pi_reset(foc_pi_t *pi)
{
    pi->integ = 0;
}

q15_t foc_pi_run(foc_pi_t *pi, q15_t ref, q15_t fb)
{
    int32_t err = (int32_t)ref - (int32_t)fb;

    /* 积分，并把积分项限制在输出范围内（抗饱和） */
    pi->integ += pi->ki * err;
    int32_t imax = (int32_t)pi->out_max << 10;
    int32_t imin = (int32_t)pi->out_min << 10;
    if (pi->integ > imax) pi->integ = imax;
    if (pi->integ < imin) pi->integ = imin;

    int32_t out = (pi->kp * err + pi->integ) >> 10;
    if (out > pi->out_max) out = pi->out_max;
    if (out < pi->out_min) out = pi->out_min;
    return (q15_t)out;
}
