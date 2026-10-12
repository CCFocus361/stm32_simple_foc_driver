#include "foc_math.h"
#include "sin_table.h"

/* 整数开方（牛顿迭代前的位运算法），用于矢量限幅，避免引入浮点 */
static uint32_t isqrt32(uint32_t x)
{
    uint32_t res = 0;
    uint32_t bit = 1UL << 30;
    while (bit > x) bit >>= 2;
    while (bit != 0) {
        if (x >= res + bit) {
            x -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

static q15_t sin_lookup(uint16_t angle)
{
    uint16_t idx  = angle >> 8;          /* 0..255 */
    int32_t  frac = angle & 0xFF;        /* 0..255 */
    int32_t  s0   = foc_sin_table[idx];
    int32_t  s1   = foc_sin_table[(idx + 1) & (FOC_SIN_TABLE_SIZE - 1)];
    return (q15_t)(s0 + (((s1 - s0) * frac) >> 8));
}

foc_trig_t foc_sincos(uint16_t angle)
{
    foc_trig_t t;
    t.sin = sin_lookup(angle);
    t.cos = sin_lookup((uint16_t)(angle + 16384u)); /* +90° */
    return t;
}

foc_ab_t foc_clarke(q15_t ia, q15_t ib)
{
    foc_ab_t o;
    o.alpha = ia;
    /* beta = (ia + 2*ib)/√3，1/√3 ≈ 18919/32768 */
    o.beta = foc_sat_q15((((int32_t)ia + 2 * (int32_t)ib) * 18919) >> 15);
    return o;
}

foc_dq_t foc_park(foc_ab_t in, foc_trig_t t)
{
    foc_dq_t o;
    o.d = foc_sat_q15((((int32_t)in.alpha * t.cos) + ((int32_t)in.beta * t.sin)) >> 15);
    o.q = foc_sat_q15(((-(int32_t)in.alpha * t.sin) + ((int32_t)in.beta * t.cos)) >> 15);
    return o;
}

foc_ab_t foc_inv_park(foc_dq_t in, foc_trig_t t)
{
    foc_ab_t o;
    o.alpha = foc_sat_q15((((int32_t)in.d * t.cos) - ((int32_t)in.q * t.sin)) >> 15);
    o.beta  = foc_sat_q15((((int32_t)in.d * t.sin) + ((int32_t)in.q * t.cos)) >> 15);
    return o;
}

void foc_svpwm(foc_ab_t v, uint16_t period, uint16_t ccr[3])
{
    /* 反 Clarke 得三相电压；√3/2 ≈ 28378/32768 */
    int32_t half_alpha = (int32_t)v.alpha >> 1;
    int32_t s3b = ((int32_t)v.beta * 28378) >> 15;
    int32_t va = v.alpha;
    int32_t vb = -half_alpha + s3b;
    int32_t vc = -half_alpha - s3b;

    /* 最小-最大零序注入：减去 (max+min)/2，使三相居中 */
    int32_t vmax = va, vmin = va;
    if (vb > vmax) vmax = vb;
    if (vc > vmax) vmax = vc;
    if (vb < vmin) vmin = vb;
    if (vc < vmin) vmin = vc;
    int32_t off = -((vmax + vmin) >> 1);

    int32_t half = (int32_t)period >> 1;
    int32_t v3[3] = { va + off, vb + off, vc + off };

    for (int i = 0; i < 3; i++) {
        int32_t d = half + ((v3[i] * (int32_t)period) >> 15);
        if (d < 0) d = 0;
        if (d > (int32_t)period) d = period;
        ccr[i] = (uint16_t)d;
    }
}

foc_dq_t foc_limit_vector(foc_dq_t v, q15_t vmax)
{
    int32_t d = v.d, q = v.q;
    uint32_t mag2 = (uint32_t)(d * d) + (uint32_t)(q * q);
    uint32_t lim2 = (uint32_t)vmax * (uint32_t)vmax;
    if (mag2 <= lim2 || mag2 == 0) return v;

    uint32_t mag = isqrt32(mag2);
    v.d = (q15_t)((d * (int32_t)vmax) / (int32_t)mag);
    v.q = (q15_t)((q * (int32_t)vmax) / (int32_t)mag);
    return v;
}
