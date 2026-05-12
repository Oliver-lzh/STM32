# Firmware Branch: feature/internal-start-acquisition-20260512

## Created: 2026-05-12

## 本分支实现的功能

### 1. 内部启动采集调试入口 (AppRadarDebugAcquisition)

**文件:**
- `sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarDebugAcquisition.c`
- `sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarDebugAcquisition.h`

**功能:**
- 在 `DEBUG_TEXT` 模式下自动配置 BGT60TR13C 雷达并启动 Raw Data 采集
- 不依赖官方 Radar Fusion GUI 上位机即可触发数据入口
- 配置 128 chirps/frame, 64 samples/chirp, 3 RX channels, 12-bit ADC (Packed12)
- 输出状态日志: `debug-acq,config,start` → `debug-acq,profile,ok` → `debug-acq,data-config,ok` → `debug-acq,start,ok`

### 2. Frame 数据探测 (AppRadarProbe)

**文件:**
- `sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarProbe.c`
- `sources/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarProbe.h`

**功能:**
- 在 `Board_dataCallback()` 处拦截每帧雷达数据
- 限频打印: 前 4 帧全打印，之后每 16 帧打印一次
- 输出格式: `probe,f=<counter>,cnt=<size>,ch=<channel>,ts=<timestamp>,dt=<delta>,fmt=<format>,head=<hex>`

### 3. 输出模式切换 (BoardOutput)

**文件:**
- `sources/targets/atmel/subprojects/RadarBaseboardMCU7/BoardOutput.c`
- `sources/targets/atmel/subprojects/RadarBaseboardMCU7/BoardOutput.h`

**功能:**
- `BOARD_OUTPUT_MODE_DEBUG_TEXT` (默认): 普通串口调试模式
- `BOARD_OUTPUT_MODE_HOST_PROTOCOL`: 官方上位机协议模式
- 支持 `/mode debug` 和 `/mode host` 命令切换

### 4. 板级初始化集成 (Board.c)

**修改:**
- 在 `Board_Constructor()` 中注册 `AppRadarDebugAcquisition_Constructor()`
- 在 `Board_run()` 中调用 `AppRadarDebugAcquisition_run()`

## 关键参数

| 参数 | 值 |
|------|-----|
| samplesPerChirp | 64 |
| chirpsPerFrame | 128 |
| rxChannels | 3 |
| frameSize (Packed12) | 18432 bytes |

## 编译验证

```bash
cd sources/targets/atmel/subprojects/RadarBaseboardMCU7
mingw32-make BUILD_TYPE=debug
```

## 相关文档

- `skills/内部启动采集调试入口开发技能.md`
