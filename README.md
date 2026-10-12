# STM32F103 FOC 三相驱动板

基于 STM32F103C8T6 的开源 FOC（磁场定向控制）项目，包含**三相驱动板硬件**与**嵌入式固件**。

> 状态：🚧 开发中。当前阶段见 [docs/roadmap.md](docs/roadmap.md)。

## 目标

| 项目 | 目标值（草案，随实测修改） |
|---|---|
| MCU | STM32F103C8T6（Cortex-M3, 72 MHz, 无 FPU） |
| 母线电压 | 12–24 V |
| 持续 / 峰值电流 | ~10 A / ~20 A |
| 电流采样 | 三相低侧电阻（或集成驱动内置放大器） |
| 位置反馈 | 磁编码器（SPI） |
| 控制 | Q15 定点 FOC：电流环 → 速度环 |
| 电机 | 2204 / 2208 云台电机或小型航模无刷电机 |

## 仓库结构

```
hardware/   KiCad 原理图/PCB、BOM、设计计算      (CERN-OHL-P-2.0)
firmware/   Keil + CubeMX 工程与 FOC 库           (MIT)
  foc/        FOC 数学库（与硬件无关，可在电脑上测试）
  app/        与硬件相关的应用代码、示例
  test/       主机端单元测试（gcc 即可跑）
tools/      脚本（正弦表生成等）
docs/       路线图、调试记录、Git 入门、采购清单
```

## 快速开始（固件）

1. 用 CubeMX 按 [firmware/README.md](firmware/README.md) 配置并生成 Keil 工程。
2. 把 `firmware/foc/` 加入工程，参考 `firmware/app/openloop_example.c` 跑开环。
3. 没有电机的话先看 [docs/shopping-list.md](docs/shopping-list.md)。

## 在电脑上测试数学库

```bash
cd firmware/test
gcc -O2 -I../foc test_foc.c ../foc/foc_math.c ../foc/foc_pi.c -lm -o test_foc && ./test_foc
```

## 贡献

欢迎 Issue 与 PR。提交前请确保主机端测试通过。

## 许可证

- 软件：[MIT](LICENSE)
- 硬件：CERN-OHL-P-2.0，见 [hardware/LICENSE_NOTE.md](hardware/LICENSE_NOTE.md)
