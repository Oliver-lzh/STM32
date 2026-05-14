# 阶段 A: AppRangeFft - Range FFT 技术开发文档

## 1. 概述

**目标:** 在 Cortex-M7 上实现单 chirp Range FFT，找到目标 range bin

**输入:** Packed12 格式的雷达数据 (64 samples/chirp, 96 bytes/chirp)
**输出:** `fft,bin=<peak_bin>,mag=<log2mag>` 调试信息

---

## 2. TI vs 英飞凌 数据格式对比

### 2.1 TI (xWR16xx) - 复数 I/Q 格式

TI radar 使用 I/Q 接收机，数据类型为 `cmplx16ReIm_t`：

```c
typedef struct cmplx16ReIm_t {
    int16_t real;  // I component
    int16_t imag;  // Q component
} cmplx16ReIm_t;
```

**特点：**
- ADC 采样得到复数 (I+jQ)
- FFT 输入是复数，`arm_cfft_f32()` (复数 FFT)
- 可以直接计算 `atan2(Q, I)` 得到相位
- 动态范围更好，3dB SNR 改善

### 2.2 英飞凌 BGT60TR13C - 实数格式

英飞凌使用实数 ADC (Packed12)，证据在 `DataAvian.c:383`：

```c
// two 12-bit samples are packed into three 8-bit words
readoutCount = (readoutCount * 3 / 2);
```

**特点：**
- ADC 采样得到实数幅度
- FFT 使用 `arm_rfft_fast_f32()` (实数 FFT)
- 虽然输入是实数，但 FFT 输出仍是复数 (real[j], imag[j])
- 可以从 FFT 结果提取相位 `atan2(imag, real)`

### 2.3 实数 vs 复数对算法的影响

**对呼吸/心率算法的影响：**

| 方面 | 实数格式 | 复数 I/Q 格式 |
|------|---------|--------------|
| 相位提取 | `atan2(imag, real)` | `atan2(Q, I)` |
| 动态范围 | 较低 | 较高 (~3dB) |
| 运动方向 | 无法区分 ±Doppler | 可区分 |
| FFT 类型 | `arm_rfft_fast_f32()` | `arm_cfft_f32()` |

**核心算法（呼吸/心率检测）两者都能做，原因：**

1. 呼吸/心率检测依赖的是**相位随时间的变化**
2. 复数 FFT 输出都有实部和虚部，可以计算相位
3. 公式：`phase(t) = atan2(imag(t), real(t))`
4. 后续的 unwrap → PCA → 滤波 → BPM 流程相同

**主要区别在于 SNR 和运动方向辨别：**
- 实数格式 SNR 稍低，但算法上可接受
- 运动方向（深吸气vs呼气）在复数格式更清晰，实数格式只能看到幅值变化

---

## 3. 接口设计

### AppRangeFft.h

```c
#ifndef APP_RANGE_FFT_H_
#define APP_RANGE_FFT_H_ 1

#include <stdint.h>

void AppRangeFft_initialize(void);
void AppRangeFft_process(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp);

#endif /* APP_RANGE_FFT_H_ */
```

### 调用关系

```
Board_dataCallback(payload=36864 bytes)
    │
    ├── AppRadarProbe_onFrame()     [现有，frame 元数据打印]
    │
    └── AppRangeFft_process()        [NEW，处理第一个 chirp FFT]
          │
          ├── unpack_packed12_to_float()   [解压缩 96 bytes → 64 float]
          ├── apply_hanning_window()      [加窗]
          ├── arm_rfft_fast_f32()          [CMSIS-DSP FFT]
          ├── arm_cmplx_mag_f32()          [计算幅值]
          └── find_peak_bin()              [找峰值 bin]
```

---

## 3. 文件组织

| 文件 | 操作 | 说明 |
|------|------|------|
| `AppRangeFft.h` | 新增 | 公共接口 |
| `AppRangeFft.c` | 新增 | FFT 实现 |
| `Board.c` | 修改 | 调用 AppRangeFft_process() |

---

## 4. 数据格式

### 4.1 Packed12 解压缩

每个 sample 12 bits，压缩到 1.5 bytes：

```
byte[i*3+0] = sample_i[7:0]
byte[i*3+1] = (sample_i[11:8] << 4) | (sample_{i+1}[3:0])
byte[i*3+2] = sample_{i+1}[11:4]
```

解码公式：
- **偶数 sample**: `sample = lowByte | ((highNibble >> 4) << 8)`
- **奇数 sample**: `sample = (lowNibble & 0x0F) | ((highByte & 0xFF) << 4)`

### 4.2 帧数据布局 (36864 bytes/frame)

```
Byte offset = (chirpIndex * 3 + rxChannel) * 96 + byteOffsetInChirp

Chirp 0, RX0: offset 0   ~ 95   (96 bytes)
Chirp 0, RX1: offset 96  ~ 191  (96 bytes)
Chirp 0, RX2: offset 192 ~ 287  (96 bytes)
Chirp 1, RX0: offset 288 ~ 383  (96 bytes)
...
Chirp 127, RX2: offset 36864 - 96 ~ 36863
```

### 4.3 FFT 输出格式 (实数输入)

`arm_rfft_fast_f32()` 对实数输入的输出结构与复数 FFT 不同：

```
输出索引:  [0]    [1]    [2]    [3]    ... [62]  [63]
对应:     DC(0)  f0(I)  f0(Q)  f1(I)  ... f31(I) f31(Q)

实际幅值计算:
for (i = 1; i < 32; i++) {
    float realVal = fftOutput[i * 2];
    float imagVal = fftOutput[i * 2 + 1];
    float mag = sqrtf(realVal*realVal + imagVal*imagVal);
    // 找最大值 bin
}
```

**重要：** 虽然输入是实数，但 FFT 输出仍包含正频率的 I/Q 信息，可用于相位提取。

### 4.4 相位提取 (可用于阶段 B)

从 FFT 结果提取相位：
```c
float phase = atan2f(imagVal, realVal);  // atan2(Q, I)
```

这为后续的 unwrap → PCA → 呼吸/心率滤波链路提供数据基础。

---

## 5. 算法步骤

### 5.1 unpack_packed12_to_float()

```c
static void unpack_packed12_to_float(const uint8_t *packed, float *samples, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        uint32_t byteIndex = (i * 3) / 2;
        if ((i & 1) == 0) {
            // 偶数 sample
            uint8_t lowByte = packed[byteIndex];
            uint8_t highNibble = packed[byteIndex + 1] >> 4;
            samples[i] = (float)((uint16_t)lowByte | ((uint16_t)highNibble << 8));
        } else {
            // 奇数 sample
            uint8_t lowNibble = packed[byteIndex] & 0x0F;
            uint8_t highByte = packed[byteIndex + 1];
            samples[i] = (float)((uint16_t)lowNibble | ((uint16_t)highByte << 4));
        }
    }
}
```

### 5.2 generate_hanning_window() - 参考 TI 递归方法

```c
#define ONE_Q15  0x8000  // Q15 格式的 1.0

static void generate_hanning_window(int16_t *window, uint32_t size)
{
    float phi = 2.0f * 3.14159265358979f / ((float)size - 1.0f);
    float eR = cosf(phi);   // cos(phi)
    float eI = sinf(phi);   // sin(phi)
    float tmpR;

    for (uint32_t i = 0; i < size; i++) {
        int32_t winVal = (int32_t)((ONE_Q15 * 0.5f * (1.0f - eR)) + 0.5f);
        if (winVal >= ONE_Q15) {
            winVal = ONE_Q15 - 1;
        }
        window[i] = (int16_t)winVal;

        // 递归更新: eR_new = eR*cos(phi) - eI*sin(phi)
        tmpR = eR;
        eR = eR * eR - eI * eI;  // cos(2*phi) 简化
        eI = 2.0f * tmpR * eI;   // sin(2*phi) 简化
    }
}
```

### 5.3 apply_window_and_fft() - 参考 TI DSP_fft16x16

```c
// 输入加窗 (参考 mmwavelib_windowing16x16)
for (uint32_t i = 0; i < fftSize; i++) {
    // Q15 定点乘法
    int32_t product = (int32_t)input[i] * (int32_t)window[i];
    // 右移 15 位还原浮点
    input[i] = (float)product / (float)ONE_Q15;
}

// 定点 FFT (类似 DSP_fft16x16)
// 使用 arm_rfft_fast_f32 替代 DSP_fft16x16
arm_rfft_fast_f32(&m_fftInstance, input, fftOutput);
```

### 5.4 compute_log2_magnitude() - 参考 TI mmwavelib_log2Abs32

```c
static void compute_log2_magnitude(const float *fftOutput, float *magOut, uint32_t fftSize)
{
    // 对于实数输入的 RFFT，输出格式为 [real0, imag0, real1, imag1, ...]
    // 我们只取正频率部分 (bin 1 ~ fftSize/2 - 1)
    for (uint32_t i = 1; i < fftSize / 2; i++) {
        float realVal = fftOutput[i * 2];     // 实部
        float imagVal = fftOutput[i * 2 + 1]; // 虚部
        float magSquared = realVal * realVal + imagVal * imagVal;
        // log2 幅值，动态范围更大
        // mag = sqrt(real² + imag²)
        // log2(mag) = 0.5 * log2(magSquared)
        float magLog2 = 0.5f * log10f(magSquared + 1e-10f) * 3.321928f;
        magOut[i] = magLog2 + 60.0f;  // 调整刻度便于阅读
    }
}
```

### 5.5 find_peak_bin()

```c
static uint32_t find_peak_bin(const float *magnitudes, uint32_t startBin, uint32_t endBin)
{
    uint32_t peakBin = startBin;
    float peakVal = magnitudes[startBin];

    for (uint32_t i = startBin + 1; i <= endBin; i++) {
        if (magnitudes[i] > peakVal) {
            peakVal = magnitudes[i];
            peakBin = i;
        }
    }
    return peakBin;
}
```

---

## 6. 静态缓冲区

```c
#define APP_RANGE_FFT_FFT_SIZE           (64u)
#define APP_RANGE_FFT_SAMPLES_PER_CHIRP   (64u)
#define APP_RANGE_FFT_BYTE_COUNT          (96u)   /* 64 * 12 / 8 */
#define APP_RANGE_FFT_DEBUG_RATE_DIVISOR  (16u)
#define APP_RANGE_FFT_ONE_Q15            (0x8000)  /* Q15 格式的 1.0 */

static int16_t  m_hanningWindow[APP_RANGE_FFT_FFT_SIZE];  /* Q15 格式，与 TI 一致 */
static arm_rfft_fast_instance_f32 m_fftInstance;
static float   m_fftInput[APP_RANGE_FFT_FFT_SIZE];
static float   m_fftOutput[APP_RANGE_FFT_FFT_SIZE];
static float   m_magnitudes[APP_RANGE_FFT_FFT_SIZE / 2];  /* log2 幅值 */
static uint32_t m_frameCounter = 0u;
static bool    m_initialized = false;
```

---

## 7. 调试输出

限频：每 16 帧输出一次，mag 使用 log2 刻度（与 TI 一致）

```
fft,bin=5,mag=45.2
fft,bin=10,mag=48.7
```

---

## 8. Board.c 集成

```c
// Board_dataCallback 中
void Board_dataCallback(void *arg, uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp)
{
    // 现有调用
    AppRadarProbe_onFrame(payload, count, channel, timestamp);

    // 新增：FFT 处理 (使用第一个 chirp, RX0)
    if (BoardOutput_getMode() == BOARD_OUTPUT_MODE_DEBUG_TEXT) {
        const uint32_t chirpByteOffset = 0;  // Chirp 0, RX0
        AppRangeFft_process(&payload[chirpByteOffset], 96, channel, timestamp);
    }
}
```

---

## 9. 验证方法

### 9.1 距离测试 (与 TI FFT 输出对照)

```
你站在雷达前 0.5m → fft,bin=5,mag=xx.x
你站在雷达前 1.0m → fft,bin=10,mag=xx.x
你站在雷达前 2.0m → fft,bin=20,mag=xx.x
```

峰值 bin 应随距离线性变化。mag 使用 log2 刻度，与 TI 输出可比。

### 9.2 峰值不变测试

固定人体在雷达前 1m 处，多次采样 bin 应稳定在同一个值。

### 9.3 验证 log2 幅值计算

- 相同位置多次测量，log2 幅值应在相近范围内
- 幅值随距离增加应下降（符合雷达方程）

---

## 10. 技术依赖

- **CMSIS-DSP**: `arm_rfft_fast_f32()`, `arm_rfft_fast_init_f32()`
- **头文件**: `<arm_math.h>` (路径: `thirdparty/CMSIS/Lib/GCC`)
- **库**: `arm_cortexM7lfsp_math_softfp.a` (已在 config.mk 中配置)
- **参考 TI**: `dss_data_path.c` 中 `MmwDemo_interChirpProcessing()` 函数

---

## 11. 注意事项

1. **只处理第一个 chirp**: 当前阶段只取 Chirp 0, RX0，后续阶段会扩展到多 chirp/多 RX
2. **限频输出**: 每 16 帧输出一次，避免刷屏
3. **初始化检查**: 如果未初始化会自动调用 `AppRangeFft_initialize()`
4. **不改动现有模块**: AppRadarProbe.c 保持不变，只在 Board.c 新增调用
5. **Q15 窗口格式**: TI 使用 Q15 定点窗口系数，浮点 ARM 可用 float 版本但保持计算方式一致
6. **log2 幅值 vs 线性幅值**: TI 使用 log2 幅值动态范围更大，建议保持一致便于后续比对
7. **实数 vs 复数**: BGT60TR13C 是实数 ADC，但 FFT 输出仍有 I/Q，可用 `atan2(imag, real)` 提取相位用于呼吸/心率检测
8. **相位提取链路**: 实数 FFT 的输出同样可用于阶段 B 的 unwrap → PCA → 呼吸/心率滤波，与 TI 算法兼容