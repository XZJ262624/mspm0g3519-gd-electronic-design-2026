# MSPM0G3519 · 2026 广东省电子设计竞赛

基于 **TI MSPM0G3519 + Code Composer Studio（CCS）** 的省电赛项目代码，项目获 **2026 年广东省电子设计竞赛省一等奖**。比赛与获奖信息由作者提供。

本仓库发布 `P20_M0G3519_Ramp` 中的底盘控制与巡线程序：16 路模拟灰度采集、硬件 QEI 测速、电机 PID、速度斜坡和任务通信。工程内部名称保留为 `P20_M0G3519_Super_Ramp`。

## 主要功能

- 16 路灰度模块通过地址选择与单 ADC 轮询采集，完成标定、归一化、位置计算和巡线控制。
- AT8236 双电机驱动、双路硬件 QEI、速度闭环与目标速度斜坡。
- 陀螺仪串口、无线数据回传、任务选择与通信。
- IPS200 屏幕显示、按键、蜂鸣器和任务计时。
- Python 灰度标定辅助工具。

## 代码结构

| 目录/文件 | 内容 |
| --- | --- |
| [firmware/main.c](firmware/main.c) | 初始化、任务调度、显示和数据发送 |
| [firmware/main.syscfg](firmware/main.syscfg) | 引脚、时钟、ADC、定时器、QEI 和串口配置 |
| [firmware/Application/](firmware/Application/) | 灰度巡线和任务协议 |
| [firmware/Algorithm/](firmware/Algorithm/) | 速度斜坡 |
| [firmware/Hardware/](firmware/Hardware/) | 电机、编码器、PID、陀螺仪、显示等 |
| [firmware/Driver/](firmware/Driver/) | 串口底层封装 |
| [firmware/System/](firmware/System/) | 板级映射、中断、时钟 |
| [firmware/tools/gray16_calibrate.py](firmware/tools/gray16_calibrate.py) | 灰度标定工具 |

## 开发环境与构建

工程配置为 **MSPM0G3519 / LQFP-64(PM)**、TI Arm Clang **4.0.3.LTS**、MSPM0 SDK **2.11.00.07**、SysConfig **1.28.0**；使用 CCS 的 Theia 工程格式与 XDS110 调试器配置。

1. 安装对应工具和 SDK，使用 CCS 工程导入功能选择 `firmware/`。
2. 检查工程属性里的 SDK、编译器、SysConfig 和目标配置 `targetConfigs/MSPM0G3519.ccxml`。
3. 打开 `main.syscfg` 确认接线并构建；生成代码、启动文件和链接依赖由 SysConfig/SDK 提供。
4. 结合 `System/Board_Config.h` 核对硬件，并在 `Application/Gray_Track.c`、`Hardware/PID.c` 等文件中调整标定和控制参数。
5. 下载后验证传感器读数、编码器方向与任务通信，再进行整车调试。

## 文档与版本差异

[开发过程](docs/DEVELOPMENT.md) 记录从 G3507 重构迁移到 G3519、QEI、通信、巡线和任务集成的过程。

原附带的 [任务选择说明](firmware/README_TaskSelect.md) 和 [16 路灰度标定说明](firmware/README_Gray16_Calibration.md) 属于较早阶段资料，开头已注明与当前代码的差异。当前代码允许任务号 **1–14**，任务发送帧为 **7 字节**；巡线开关 `LQ_GRAY_TRACK_RUN_ENABLE = 1U`，原始灰度输出开关 `GRAY_TELEMETRY_SEND_RAW_ADC = 0U`。以当前源文件为准，不应将旧文档的“标定模式”视为本版本默认状态。

## 归档范围

仅包含给定工程中的 MCU 侧程序及标定工具。SDK、编译产物、原理图、比赛报告、获奖证书及外部通信设备程序未随本仓库发布。未进行实车测试。

## 来源与许可

保留原文件中的作者及第三方声明。TI SDK 为外部依赖，按其自身许可使用；本次归档未为原创代码新增许可证。

## 发布验证

已从发布副本重新生成 SysConfig 配置，并完成全部应用源码、生成代码和启动文件的编译与链接，无编译警告。详见 [验证记录](docs/VALIDATION.md)。
