/**
 * foc_pi.h — 定点 PI 控制器（带积分抗饱和）
 *
 * 增益为 Q10 格式（1024 = 1.0）。
 *   out = kp*err + ∫ki*err，结果限幅到 [out_min, out_max]。
 * err 与输出均为 Q15 量。kp 建议 < 16384（即 16.0），以免 32 位溢出。
 * ki 已经包含采样周期：ki = Ki_连续 * Ts。
 */
#ifndef FOC_PI_H
#define FOC_PI_H

#include "foc_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t kp;       /* Q10 */
    int32_t ki;       /* Q10，已乘 Ts */
    int32_t integ;    /* 积分状态，Q25（Q15 量 × Q10） */
    q15_t   out_min;
    q15_t   out_max;
} foc_pi_t;

void  foc_pi_init(foc_pi_t *pi, int32_t kp_q10, int32_t ki_q10, q15_t out_min, q15_t out_max);
void  foc_pi_reset(foc_pi_t *pi);
q15_t foc_pi_run(foc_pi_t *pi, q15_t ref, q15_t fb);

#ifdef __cplusplus
}
#endif

#endif /* FOC_PI_H */
