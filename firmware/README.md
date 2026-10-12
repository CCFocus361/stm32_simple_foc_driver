# 固件

开发环境：**Keil MDK + STM32CubeMX**，目标 STM32F103C8T6（72 MHz，Cortex-M3，无 FPU）。

## 目录

```
firmware/
├── foc/     FOC 数学库（纯 C，与硬件无关，主机可测试）
├── app/     与硬件相关的代码、示例
├── test/    主机端单元测试
└── (CubeMX 生成的工程放在这里：*.ioc、Core/、Drivers/、MDK-ARM/)
```

## 为什么用定点（Q15）

F103 没有 FPU，`sinf()` / float 乘除用软件实现，一次要几十到上百个周期，20 kHz 控制周期（50 µs ≈ 3600 个时钟）里放不下完整电流环。`foc/` 全程整数运算 + 正弦查表。

## CubeMX 配置清单

### 时钟
- HSE 8 MHz → PLL ×9 → SYSCLK **72 MHz**；APB2 = 72 MHz，TIM1 时钟 72 MHz。

### TIM1：三相互补 PWM（20 kHz）
| 项目 | 设置 |
|---|---|
| Channel 1/2/3 | PWM Generation CHxN（带互补输出） |
| Counter Mode | **Center Aligned mode 1** |
| Prescaler | 0 |
| Counter Period (ARR) | **1799**（72 MHz / (2×1800) = 20 kHz） |
| Repetition Counter | 0 |
| Dead Time | 先设约 0.5–1 µs（DTG≈36–72，72 MHz 下每步 ≈13.9 ns）。**必须用示波器核对，并结合你的 MOSFET/驱动芯片数据手册调整** |
| Break | 先 Disable，等硬件有过流保护信号后再启用 |
| Output Idle / Off-state | 先保持默认，之后确认故障时桥臂状态是否安全 |
| 引脚 | CH1/2/3 → PA8/PA9/PA10；CH1N/2N/3N → PB13/PB14/PB15 |

> 用 DRV8313 这类模块时，它的 IN1–3 直接接 PWM，由芯片内部处理，不用互补输出；这时只用 CH1/2/3，另用一个 GPIO 控制 EN。按你实际模块调整。

### 控制中断
- 先用 TIM1 更新中断（NVIC 使能 TIM1 update），在回调里跑控制环；后续做电流采样时改为 ADC 注入转换完成中断。

### ADC（阶段 1.7 再配）
- ADC1 + ADC2 **双 ADC 同步注入模式**（Dual injected simultaneous），同时采两相电流。
- 注入触发：**Timer 1 CC4 event**，在 CubeMX 里把 TIM1 Channel4 设为 Output Compare No Output（只产生触发事件）。
- 采样时刻要落在下桥导通的中点。中心对齐模式下，该时刻在计数峰值还是谷值，取决于 PWM 模式（PWM1/PWM2）极性。**以示波器同时看 PWM 与采样触发信号确认，不要凭记忆。**
- 采样时间选较短（如 7.5 或 13.5 周期）。

### SPI（编码器，阶段 1.3）
- Master，SPI Mode 1（CPOL=0, CPHA=1，AS5047P），波特率先低（≈2–4 MHz）。

### 串口
- USART1 作为调试输出。

## 工程建议

- `.ioc` 必须提交；改配置后重新生成代码，用户代码写在 `/* USER CODE BEGIN */` 区间内。
- 新代码尽量放 `foc/` 与 `app/`，不要散落在 CubeMX 生成文件里，便于复用与测试。

## 主机端测试

```bash
cd firmware/test
gcc -O2 -I../foc test_foc.c ../foc/foc_math.c ../foc/foc_pi.c -lm -o test_foc && ./test_foc
```

修改 `foc/` 后先跑这个，通过再下载到板子。
