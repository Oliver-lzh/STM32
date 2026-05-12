# TI 生命体征算法移植到 RadarBaseboardMCU7 技术规划

## 1. 文档目标

本文档用于规划如何把 TI `vitalSigns_16xx_dss` demo 中的呼吸率、心率处理思路，分阶段移植到当前英飞凌 `RadarBaseboardMCU7` 固件。

本阶段只做技术规划和开发拆分，不修改 `Board.c`、`BoardOutput.c`、`DataAvian.c` 或 TI demo 源码。

第一版目标不是完整复刻 TI 的 DSS、EDMA、radarCube 架构，而是先在英飞凌 MCU 固件里跑通一条可验证的生命体征处理链路：

```text
原始数据探测
-> chirp/frame 解析
-> Range FFT
-> 轻量 range-bin I/Q 缓存
-> 目标距离 bin 选择
-> 相位 unwrap
-> 呼吸/心率滤波和估计
-> BoardOutput 输出
```

## 2. 当前工程关键结论

### 2.1 英飞凌侧数据入口

当前 RadarBaseboardMCU7 的雷达数据入口主要在 Avian 数据链路中。

需要重点跟踪的源码路径：

```text
sources/sources/stratula/library/platform/DataAvian.c
sources/targets/atmel/subprojects/RadarBaseboardMCU7/Board.c
```

已定位的数据流方向：

```text
DataAvian_readData()
-> 从 Avian FIFO/SPI 读取雷达数据
-> 写入 DataAvian 内部数据队列或缓冲
-> DataAvian_run()
-> frameCallback(data, frameSize, index, timestamp)
-> Board_dataCallback(payload, count, channel, timestamp)
```

`Board_dataCallback(...)` 是当前最适合做第一阶段数据探测的位置，因为它已经拿到了上层能看到的 `payload`、`count`、`channel`、`timestamp`。

第一步必须确认：

- `payload` 是单个 chirp 的 ADC 数据。
- 还是多个 chirp 聚合后的 frame 数据。
- 还是已经被上位机或固件配置成某种输出格式的数据帧。
- `count` 的单位是字节数、样本数，还是协议层定义的元素数量。
- `channel` 对应的是雷达数据通道、数据队列 index，还是某个设备/虚拟通道编号。

在没有确认 `payload` 结构前，不应该直接把 TI 的生命体征算法接进去。算法入口必须建立在明确的数据格式上。

### 2.2 TI 侧处理方式

需要重点跟踪的 TI demo 源码路径：

```text
vitalSigns_16xx_dss/vitalSigns_16xx_dss/dss_data_path.c
vitalSigns_16xx_dss/vitalSigns_16xx_dss/dss_main.c
vitalSigns_16xx_dss/vitalSigns_16xx_dss/dss_vitalSignsDemo_utilsFunc.c
vitalSigns_16xx_dss/vitalSigns_16xx_dss/dss_pca_vitalsigns.c
```

TI demo 的核心思路：

- 每个 chirp 进入数据路径后，先做 Range FFT。
- 多个 chirp 组成一帧，也就是一个 frame/cube 的处理单位。
- 一帧内 chirp 数量通常由配置决定：

```c
numChirpsPerFrame = (chirpEndIdx - chirpStartIdx + 1) * numLoops;
```

- 生命体征算法不是对每一个 chirp 单独输出呼吸/心率。
- 它通常在 frame 结束后，从 range FFT 结果里选出人体所在的某个 range bin。
- 每一帧取这个 range bin 的复数 `I/Q` 点，形成慢时间序列。
- 慢时间序列再用于相位提取、相位展开、呼吸滤波、心跳滤波、频谱峰值估计。

因此，TI 里“最小生命体征估计单位”不是单 chirp，而是 frame。单 chirp 是 Range FFT 的输入单位，frame 是生命体征慢时间采样的更新单位。

## 3. DSP 能力和移植边界

TI `vitalSigns_16xx_dss` 跑在 TI mmWave 的 DSS 侧，使用 C674x DSP、EDMA、专用内存布局和 TI mmWave SDK 数据路径。

RadarBaseboardMCU7 使用的是 Atmel/SAM 系列 Cortex-M7 MCU。它没有 TI C674x 那种独立 DSP 核，但 Cortex-M7 本身具备：

- 单精度 FPU。
- ARM DSP 指令扩展。
- 可使用 CMSIS-DSP 做 FFT、滤波、向量运算。

所以这里的判断是：

- 可以移植生命体征算法思想。
- 可以在 MCU 上做一定规模的 Range FFT 和慢时间处理。
- 不建议完整照搬 TI 的 EDMA、radarCube、DSS/MSS 消息机制。
- 不建议一开始就在 MCU 上保存完整大 cube。
- 优先确认英飞凌固件或上位机配置是否已经能输出 Range FFT 后的复数数据。
- 如果只能拿到原始 ADC/chirp 数据，再在 Cortex-M7 上用 CMSIS-DSP 做轻量 Range FFT。

## 4. TI 与英飞凌数据流对照

### 4.1 TI demo 数据流

```text
ADC samples
-> chirp interrupt / ADCBuf
-> interChirpProcessing()
-> Range FFT
-> radarCube
-> frame done
-> interFrameProcessing()
-> 选择目标 range bin
-> 提取该 bin 的 I/Q 慢时间序列
-> 相位 unwrap
-> 呼吸/心率估计
-> TLV/UART 输出
```

代表性函数：

```text
MmwDemo_interChirpProcessing(...)
MmwDemo_interFrameProcessing(...)
```

### 4.2 当前英飞凌固件数据流

```text
Avian radar FIFO/SPI
-> DataAvian_readData()
-> DataAvian 内部缓冲/队列
-> DataAvian_run()
-> frameCallback(...)
-> Board_dataCallback(payload, count, channel, timestamp)
-> BoardOutput_onFrame(...) 或 ProtocolHandler_sendDataFrame(...)
```

当前最关键的未知点是：`Board_dataCallback(...)` 收到的 `payload` 到底对应 TI 数据流中的哪个层级。

可能性包括：

- 原始 ADC samples。
- 单 chirp 数据。
- 多 chirp 聚合数据。
- 固件或上位机配置后的完整 frame 数据。
- 已经经过某种预处理的数据。

这个未知点决定后面是否需要自己实现 Range FFT，以及如何拼 frame/cube。

## 5. 推荐移植流水线

建议按下面的顺序开发，不要一开始直接搬 TI 算法大段代码。

```text
阶段 1：数据入口探测
阶段 2：chirp/frame 结构确认
阶段 3：Range FFT 原型
阶段 4：轻量 cube / range-bin I/Q 缓存
阶段 5：生命体征慢时间算法
阶段 6：输出集成和协议保护
```

每个阶段都应该能单独验证，验证通过后再进入下一阶段。

## 6. 阶段 1：数据入口探测

### 目标

确认 `Board_dataCallback(...)` 或 `BoardOutput_onFrame(...)` 里拿到的 `payload/count/channel/timestamp` 实际含义。

### 建议入口

```text
sources/targets/atmel/subprojects/RadarBaseboardMCU7/Board.c
```

可观测位置：

```c
Board_dataCallback(void *arg,
                   uint8_t *payload,
                   uint32_t count,
                   uint8_t channel,
                   uint64_t timestamp)
```

也可以把探测逻辑放在 `BoardOutput_onFrame(...)` 后面，避免 `Board.c` 继续变重。

### 需要记录的内容

不要直接打印整个 payload。第一阶段只打印少量摘要：

```text
frame,<counter>,count=<count>,channel=<channel>,timestamp=<timestamp>
head,<前 16 或 32 字节十六进制>
delta_t,<相邻 timestamp 差值>
```

### 成功标准

- 能稳定看到 `count`。
- 能看到 `timestamp` 是否随 frame/chirp 增长。
- 能确认 `channel` 是否固定或随数据源变化。
- 能初步判断 `payload` 是否有固定长度周期。
- 不会因为串口打印太多导致雷达采集卡死。

## 7. 阶段 2：chirp/frame 结构确认

### 目标

确定英飞凌 payload 如何切分成 chirp 和 frame。

### 需要结合的信息

- 上位机或固件配置里的 chirp 数。
- 每个 chirp 的 ADC sample 数。
- RX 通道数。
- 每个 sample 的位宽。
- 是否 I/Q 交织。
- 是否多 RX 交织。
- `count` 与上述参数的倍数关系。

### 推导方向

如果 payload 是原始 ADC 数据，可以先用下面的关系验证：

```text
单 chirp 字节数 ~= adcSamples * rxCount * sampleBytes * iqFactor
frame 字节数 ~= 单 chirp 字节数 * chirpsPerFrame
```

其中：

- `sampleBytes` 可能是 2 字节或 4 字节。
- `iqFactor` 如果是实数采样可为 1，如果是 I/Q 复数采样可为 2。
- 最终以实际 payload 和配置为准。

### 成功标准

- 能说明一帧里有多少 chirp。
- 能说明每个 chirp 有多少 ADC sample。
- 能说明 RX/IQ 排列方式。
- 能把 `payload` 中某一段稳定解释为一个 chirp。

## 8. 阶段 3：Range FFT 原型

### 目标

在 Cortex-M7 上验证单 chirp Range FFT。

### 建议实现方式

优先使用 CMSIS-DSP：

```text
arm_rfft_fast_f32
或
arm_cfft_q15 / arm_cfft_q31
```

第一版建议先用浮点 `float32` 原型，确认算法正确后再考虑定点优化。

### 开发策略

- 先只处理一个 RX。
- 先只处理一个 chirp。
- 先只输出 FFT 幅值最大 bin。
- 不做完整 cube。
- 不做心率/呼吸估计。

### 成功标准

- 静止反射物或人体目标时，Range FFT 有稳定峰值。
- 改变目标距离时，峰值 bin 有合理变化。
- FFT 运算不会导致主循环明显卡死。
- `.map` 中 SRAM 和 Flash 余量仍然安全。

## 9. 阶段 4：轻量 cube / range-bin I/Q 缓存

### 目标

为生命体征算法准备慢时间输入，但避免保存完整大 cube。

TI demo 里可以使用 radarCube，因为它有对应的 DSS/EDMA/内存设计。RadarBaseboardMCU7 第一版不建议完整复刻。

推荐第一版只缓存目标距离附近的少量 range bin：

```text
每帧 Range FFT
-> 选择几个候选 range bin
-> 保存这些 bin 的复数 I/Q
-> 形成慢时间 ring buffer
```

### 建议缓存结构

```text
binCount: 例如 4 到 16 个候选 bin
historyLength: 例如 128 到 512 帧
每个点: complex float 或 complex q15/q31
```

### SRAM 风险

完整 cube 的内存占用可能很快变大：

```text
rangeBins * chirpsPerFrame * rxCount * complexBytes
```

第一版应该优先使用轻量缓存：

```text
candidateBins * slowTimeFrames * complexBytes
```

### 成功标准

- 能稳定保存某个 range bin 的 I/Q 历史。
- I/Q 相位会随人体微动变化。
- 不保存完整 cube 也能推进呼吸链路验证。

## 10. 阶段 5：生命体征慢时间算法

### 目标

移植 TI 的生命体征算法思想，用 range bin 的慢时间 I/Q 估计呼吸率和心率。

### 推荐处理链路

```text
range bin I/Q
-> atan2(Q, I) 得到相位
-> 相位 unwrap
-> 去直流/去趋势
-> 呼吸带通滤波
-> 心跳带通滤波
-> 慢时间 FFT 或峰值估计
-> 输出 respiration_bpm / heart_bpm
```

### TI 中可参考的算法内容

```text
dss_vitalSignsDemo_utilsFunc.c
dss_pca_vitalsigns.c
dss_data_path.c 中 vital signs 参数初始化和处理段
```

可以参考但不要直接照搬的内容：

- TI 的 EDMA 触发模型。
- TI 的 DSS 内存布局。
- TI 的 TLV 打包结构。
- TI 的 radarCube 大缓存。

### 第一版建议

- 先实现呼吸率。
- 呼吸跑通后再做心率。
- PCA 可以作为后续增强，不放在第一版关键路径。
- 先用固定 range bin 或手动选择 range bin，后面再做自动目标 bin 选择。

### 成功标准

- 固定人体目标时，相位波形随呼吸有周期变化。
- 呼吸 BPM 输出在合理范围内。
- 心率输出可以先标记为实验值，不作为第一阶段强指标。

## 11. 阶段 6：输出集成与协议保护

### 目标

把调试信息和算法结果输出给串口助手，同时保持官方上位机协议兼容。

### 输出规则

- `HOST_PROTOCOL` 模式继续走官方协议，不输出普通文本。
- `DEBUG_TEXT` 模式可以输出探测信息和算法结果。
- 后续如果新增 `ALGORITHM_OUTPUT` 模式，应与官方协议明确隔离。

建议输出格式：

```text
raw,count=<count>,channel=<channel>,timestamp=<timestamp>
fft,peak_bin=<bin>,peak_mag=<mag>
vital,breath_bpm=<value>,heart_bpm=<value>,quality=<value>
```

### 成功标准

- 官方上位机模式不被普通文本污染。
- 串口助手能看到算法结果。
- 输出频率受控，不影响采集。

## 12. 建议模块拆分

后续真正写代码时，建议不要把所有逻辑都堆到 `Board.c`。

可考虑新增应用层模块：

```text
sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarProbe.c
sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarProbe.h
sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRangeFft.c
sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRangeFft.h
sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppVitalSigns.c
sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppVitalSigns.h
```

建议职责：

- `AppRadarProbe`：只负责 payload 摘要、结构探测、少量样本打印。
- `AppRangeFft`：只负责单 chirp 或单 frame 的 Range FFT 原型。
- `AppVitalSigns`：只负责 range bin I/Q 慢时间处理和 BPM 估计。
- `BoardOutput`：只负责输出模式和文本/协议隔离。
- `Board.c`：只负责调用，不承载算法细节。

## 13. 验证计划

### 13.1 源码追踪验证

需要能追溯以下入口：

```text
Board.c
DataAvian.c
dss_data_path.c
dss_main.c
```

### 13.2 阶段成功标准

| 阶段 | 成功标准 |
| --- | --- |
| 数据入口探测 | 能看到 `count/channel/timestamp/少量 payload head`，且不阻塞采集 |
| chirp/frame 结构确认 | 能说明 payload 是单 chirp、多 chirp，还是完整 frame |
| Range FFT | 单 chirp FFT 有稳定目标峰值 |
| 轻量缓存 | 固定 range bin 的 I/Q 历史稳定更新 |
| 呼吸/心率算法 | 呼吸 BPM 能输出合理结果，心率作为后续增强 |
| 输出集成 | `DEBUG_TEXT` 有文本结果，`HOST_PROTOCOL` 不被污染 |

### 13.3 编译验证

每轮源码变更后需要检查：

- Release 编译成功。
- Debug 编译成功。
- 继续使用 Atmel Studio 自带 ARM GCC 6.3.1。
- `.bin`、`.elf`、`.map` 正常生成。
- `.map` 中 SRAM/Flash 余量可接受。

## 14. 风险表

| 风险 | 说明 | 应对方式 |
| --- | --- | --- |
| payload 格式未知 | 不知道 `Board_dataCallback()` 收到的是 raw chirp 还是 frame | 阶段 1 先做摘要打印和长度统计 |
| SRAM 不足 | 完整 radarCube 可能占用过大 | 第一版只缓存少量 range bin 的 I/Q |
| FFT 算力不足 | Cortex-M7 不是 TI C674x DSP | 先单 RX、单 chirp、低 bin 数验证，再优化 |
| 官方协议污染 | 普通文本混入 HOST_PROTOCOL 会让上位机解析失败 | 只在 `DEBUG_TEXT` 输出文本 |
| frame rate 不合适 | 生命体征慢时间采样率必须覆盖呼吸/心率频段 | 根据 timestamp 统计真实 frame rate |
| 配置不匹配 | TI demo 参数不能直接套到英飞凌板 | 以英飞凌实际 chirp/frame 配置为准 |
| 心率信噪比低 | 心跳相位变化远小于呼吸 | 先跑通呼吸，再引入滤波、PCA 或更稳的 bin 选择 |

## 15. 推荐第一轮开发任务

第一轮不要直接实现完整呼吸/心率算法。建议先做“数据入口探测”。

最小任务：

```text
在 Board_dataCallback() 或 BoardOutput_onFrame() 后面接入 AppRadarProbe。
只打印 frame counter、count、channel、timestamp、前 16/32 字节。
限制输出频率，例如每 50 或 100 帧打印一次。
确认 payload 的长度规律和 timestamp 周期。
```

第一轮输出确认后，再决定：

- 是否需要自己做 Range FFT。
- 每帧包含多少 chirp。
- 是否能只缓存目标 range bin。
- 呼吸算法的慢时间采样率是多少。

## 16. 实现边界

本规划明确不做以下事情：

- 不重写 HAL、ASF、CMSIS、SPI、DMA 底层路径。
- 不直接照搬 TI EDMA/DSS/MSS 架构。
- 不在第一版保存完整 TI radarCube。
- 不在官方 `HOST_PROTOCOL` 模式输出普通文本。
- 不声称当前已经实现生命体征算法。

第一版移植目标是：在英飞凌固件里跑通一条可观测、可验证、可逐步扩展的呼吸/心率算法链路。
