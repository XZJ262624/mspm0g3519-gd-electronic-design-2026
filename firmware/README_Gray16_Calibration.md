> **归档说明：** 本文保留了早期标定流程。P20 当前代码已启用巡线（LQ_GRAY_TRACK_RUN_ENABLE = 1U），默认输出位置而非 16 路原始 ADC（GRAY_TELEMETRY_SEND_RAW_ADC = 0U），且包含非零标定数据。下文“当前工程状态”描述的是旧阶段；执行标定前需按当前源码重新配置。

# 16路灰度接线与重新标定

## 1. 接线

| 灰度/多路复用器信号 | MSPM0G3519 |
|---|---|
| AO / SIG | PB25（ADC0 Channel 4） |
| S0 | PB17 |
| S1 | PB18 |
| S2 | PB20 |
| S3 | PB19 |
| GND | GND |
| VCC | 3.3 V |

如果模块有低有效 `EN` 引脚，将其接地。AO/SIG 电压不得超过
MSPM0G3519 的 ADC 输入范围。

S0 是最低位，S3 是最高位。代码按照二进制值 `0～15`
依次选择 D1～D16。

采集方式与原 8 路工程完全一致，仍然是单 ADC 轮询：

```text
地址0000选D1 → PB25采样
地址0001选D2 → PB25采样
...
地址1111选D16 → PB25采样
```

每次切换地址后，程序先丢弃一次 ADC 转换结果，再对同一路
有效采样 2 次并求平均，然后才切换下一路。S0～S3 只负责
选择多路复用器地址，不作为 ADC 输入；工程没有配置 16 个
并行 ADC 通道。

## 2. 当前工程状态

工程已经处于安全标定模式：

- `LQ_GRAY_CHANNEL_COUNT = 16`
- `LQ_GRAY_TRACK_RUN_ENABLE = 0U`
- `GRAY_TELEMETRY_SEND_RAW_ADC = 1U`
- 黑底、白底标定数组暂时全部为 `0U`
- 电机目标保持为 0
- 无线串口每 50 ms 发送一行 D1～D16 原始 ADC

无线一行中的 16 个数是完成一轮顺序扫描后保存的结果，
不是同时采集的 16 路数据。

发送格式示例：

```text
3810,3812,3809,3815,3811,3813,3814,3812,3808,3810,3816,3811,3809,3813,3812,3810
```

虽然 P15 中 D1、D8 被屏蔽，标定模式仍会读取全部 16 路，
因此可以重新判断 D1、D8 是否恢复正常。正式巡线时仍默认保留
P15 的 D1、D8 屏蔽设置。

## 3. 采集白底和黑底

1. 编译、烧录并保持车轮架空。
2. 保持灰度板安装高度、照明和比赛时一致。
3. 让 16 个探头全部位于白底上，保存至少 20 行数据到
   `white.txt`。
4. 用足够宽的哑光黑色区域覆盖全部 16 个探头，保存至少
   20 行数据到 `black.txt`。
5. 不要在两次采集之间改变探头高度、供电或增益。

## 4. 计算标定值

电脑上进入工程目录并执行：

```text
python tools/gray16_calibrate.py white.txt black.txt
```

脚本会：

- 检查每行是否恰好包含 16 路 ADC；
- 去掉每路最高、最低的少量异常点；
- 输出可以直接粘贴到 `Gray_Track.c` 的白底和黑底数组；
- 标出黑白跨度小于 100 的异常通道。

把输出值分别替换：

```c
static const uint16_t
s_lq_gray_white_adc[LQ_GRAY_CHANNEL_COUNT] = {
    /* 粘贴脚本输出 */
};

static const uint16_t
s_lq_gray_black_adc[LQ_GRAY_CHANNEL_COUNT] = {
    /* 粘贴脚本输出 */
};
```

如果某一路跨度小于 100、数值跳动很大或一直为 0，在
`Gray_Track.c` 中将对应开关设为 `0U`。其余通道设为 `1U`。

## 5. 标定完成后开启巡线

在 `Application/Gray_Track.c` 中改为：

```c
#define LQ_GRAY_TRACK_RUN_ENABLE           (1U)
```

在 `main.c` 中改为：

```c
#define GRAY_TELEMETRY_SEND_RAW_ADC        (0U)
```

此时无线串口恢复发送 `g_lq_gray_position`。

上车前先检查：

- 白底时，各有效通道 `g_lq_gray_strength[]` 接近 0；
- 黑底时，各有效通道接近 1000；
- 黑线在车体左侧时，`g_lq_gray_position < 0`；
- 黑线在车体右侧时，`g_lq_gray_position > 0`；
- `g_lq_gray_calibration_valid == true`。

如果左右符号相反，将
`s_lq_gray_position_weight[]` 的 16 个权重全部取反，
不要修改标定数组，也不要先修改 PID。
