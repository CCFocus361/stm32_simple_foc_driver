/**
 * openloop_example.c — 开环 SVPWM 示例（阶段 1.2）
 *
 * 目的：不用编码器、不用电流采样，先让电机"转起来"，同时验证
 *       TIM1 三相互补 PWM、死区、方向与 SVPWM 数学库。
 *
 * 使用方法
 *  1. CubeMX 按 firmware/README.md 配置 TIM1，并生成 Keil 工程。
 *  2. 把 foc/ 目录下 .c/.h 加入工程（Include Path 加 foc/）。
 *  3. 在 main() 里：
 *        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
 *        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
 *        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
 *        HAL_TIM_Base_Start_IT(&htim1);   // 更新中断作为控制周期（20 kHz）
 *  4. 在 HAL_TIM_PeriodElapsedCallback() 里，当 htim->Instance == TIM1 时调用 openloop_isr()。
 *
 * 安全：先用小幅值（OPENLOOP_VQ 小）、限流电源、低转速。
 */
#include "main.h"          /* CubeMX 生成，含 HAL 头文件 */
#include "foc_math.h"

extern TIM_HandleTypeDef htim1;

/* ---- 可调参数 ---- */
#define PWM_PERIOD        1799u     /* 与 CubeMX 中 TIM1 的 ARR 一致（20 kHz, 中心对齐） */
#define PWM_FREQ_HZ       20000u
#define POLE_PAIRS        7u        /* 以你的电机为准 */
#define OPENLOOP_VQ       2000      /* Q15，满量程=Vdc。2000 ≈ 6% Vdc，很小，先保守 */
#define OPENLOOP_RPM      60u       /* 机械转速 */

/* 每个控制周期电角度增量（0..65535 = 一圈）
 * 电频率 = rpm/60 * 极对数；增量 = 电频率 / 控制频率 * 65536 */
#define ANGLE_STEP  ((uint16_t)(((uint32_t)OPENLOOP_RPM * POLE_PAIRS * 65536u) / (60u * PWM_FREQ_HZ)))

void openloop_isr(void)
{
    static uint16_t theta = 0;
    uint16_t ccr[3];

    theta += ANGLE_STEP;

    foc_dq_t   vdq = { .d = 0, .q = OPENLOOP_VQ };
    foc_trig_t t   = foc_sincos(theta);
    foc_ab_t   vab = foc_inv_park(vdq, t);
    foc_svpwm(vab, PWM_PERIOD, ccr);

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr[0]);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr[1]);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr[2]);
}
