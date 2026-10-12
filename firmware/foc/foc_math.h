/**
 * foc_math.h — Q15 定点 FOC 数学库（与硬件无关）
 *
 * 约定
 *  - q15_t：-32768..32767 表示 -1.0..+1.0
 *  - 角度 uint16_t：0..65535 对应 0..2π（自然回绕，加减法天然取模）
 *  - 电压量满量程 32767 = 母线电压 Vdc（线性调制极限约为 0.577*Vdc ≈ 18919）
 *  - 电流量满量程由 ADC 标定决定（例如 32767 = 量程上限安培数）
 */
#ifndef FOC_MATH_H
#define FOC_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int16_t q15_t;

typedef struct { q15_t sin, cos; } foc_trig_t;
typedef struct { q15_t alpha, beta; } foc_ab_t;
typedef struct { q15_t d, q; } foc_dq_t;

/** 线性调制的最大电压矢量幅值（Vdc/√3，Q15） */
#define FOC_VMAX_LINEAR 18919

/** 饱和到 q15 */
static inline q15_t foc_sat_q15(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (q15_t)x;
}

/** 正弦/余弦（256 点表 + 线性插值） */
foc_trig_t foc_sincos(uint16_t angle);

/** Clarke：ia, ib → α, β（假设 ia+ib+ic=0，只需两相电流） */
foc_ab_t foc_clarke(q15_t ia, q15_t ib);

/** Park：α, β → d, q */
foc_dq_t foc_park(foc_ab_t in, foc_trig_t t);

/** 反 Park：d, q → α, β */
foc_ab_t foc_inv_park(foc_dq_t in, foc_trig_t t);

/**
 * SVPWM（最小-最大零序注入，与七段式 SVPWM 等价）。
 * @param v       α, β 电压（Q15，满量程=Vdc）
 * @param period  定时器计数周期（ARR 值，中心对齐时为峰值）
 * @param ccr     输出三相比较值数组 [A, B, C]，范围 0..period
 */
void foc_svpwm(foc_ab_t v, uint16_t period, uint16_t ccr[3]);

/** 把 (d,q) 电压矢量限制在半径 vmax 的圆内（保持方向） */
foc_dq_t foc_limit_vector(foc_dq_t v, q15_t vmax);

#ifdef __cplusplus
}
#endif

#endif /* FOC_MATH_H */
