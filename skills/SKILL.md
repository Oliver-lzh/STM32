---
name: radarbaseboard-mcu7-firmware
description: 在本仓库中处理 Infineon RadarBaseboardMCU7 固件二次开发时使用，尤其适用于 BGT60/Avian 雷达数据采集、USB CDC 调试输出、通信模式切换、固件内雷达参数配置和每帧数据算法处理。
---

# RadarBaseboard MCU7 固件 Skill

## Overview / 概览

- 本仓库是 Infineon RadarBaseboardMCU7 固件工程，目标 MCU 是 `SAMS70Q21B` Cortex-M7。
- 工程使用嵌入式 C，既可以通过 Microchip/Atmel Studio 打开，也可以通过外部 Makefile 编译。
- 当前二次开发仍然使用原 RadarBaseboard MCU7 和原 MCU，不做 MCU 迁移。
- 二次开发目标是：保留雷达采集能力，在每帧雷达数据接口上加入算法处理，并通过 USB CDC 输出调试信息或算法结果。
- 原固件默认通过 USB CDC serial 跑 Infineon vendor protocol 给上位机使用；普通串口助手直接看原数据帧通常会看到二进制内容。
- 后续开发建议保留原通信层作为回退路径，同时新增模式切换：上位机协议模式、普通文本调试模式、算法结果输出模式。

主固件中支持的雷达组件族包括：

- Avian，覆盖 BGT60TRxx 风格的雷达器件，例如 BGT60TR13C。
- LTR11。
- ATR22。
- Smartar。

单独的 `yizhi/` 目录包含独立的 `xensiv_bgt60trxx.*` 驱动文件。除非有明确替换计划，否则它只作为参考或实验代码，不直接混入主工程。

## Environment / 环境

- 推荐 IDE：Microchip Studio。
- 工程文件：`source/targets/atmel/subprojects/RadarBaseboardMCU7/RadarBaseboardMCU7.cproj`。
- 也可打开 solution：`source/targets/atmel/subprojects/RadarBaseboardMCU7/RadarBaseboardMCU7.atsln`。
- Microchip Studio 中看源码应使用 `View -> Solution Explorer`；ASF Wizard 显示 "Please select any project" 不代表工程没打开。
- 命令行编译目录：

```powershell
cd C:\Users\Oliver\Desktop\in\Firmware\source\targets\atmel\subprojects\RadarBaseboardMCU7
mingw32-make BUILD_TYPE=debug
```

- 本机 ARM GCC 路径应放在 `local_settings.mk`，不要硬编码到共享 Makefile。
- 预期输出文件：

```text
build-debug/RadarBaseboardMCU7.elf
build-debug/RadarBaseboardMCU7.hex
build-debug/RadarBaseboardMCU7.bin
build-debug/RadarBaseboardMCU7.map
```

## Firmware Layers / 固件分层

### 1. HAL 硬件抽象层

主要路径：

- `source/targets/atmel/target_platform/impl/`
- `source/targets/atmel/target_platform/contrib/ASF/`

主要功能：

- MCU 外设适配：SPI、GPIO、I2C、timer、interrupt、memory。
- USB CDC 底层收发：`target_platform/impl/serial/SerialPortImpl.c`。
- 平台初始化、系统时钟、外设初始化、bootloader 支持。

分层规则：

- HAL 只处理 MCU 和外设，不写雷达算法。
- HAL 不解析上位机协议。
- HAL 不决定 chirp 数、sample 数、frame 配置等雷达业务参数。
- 迁移到其他 MCU 时，主要重写这一层；当前二次开发仍使用原 MCU，优先不要大改这一层。

### 2. Driver 驱动层

主要路径：

- `source/sources/stratula/library/components/radar/Avian.c`
- `source/sources/stratula/library/components/radar/avian/`
- `source/targets/atmel/subprojects/RadarBaseboardMCU7/DataAvian.c`
- `source/sources/stratula/library/platform/DataAvian.h`

主要功能：

- BGT60/Avian 寄存器访问和协议封装。
- 雷达 reset、start、stop、register read/write。
- FIFO/SPI burst 数据读取。
- IRQ 触发后的数据搬运、队列缓存和 frame 拼接。
- 支持 `Raw16`、`Packed12` 等数据格式相关配置。

分层规则：

- Driver 层只负责雷达配置、数据读取、状态返回。
- Driver 层不要包含上位机 vendor protocol 格式。
- Driver 层不要直接输出普通串口文本。
- 修改数据路径时优先处理完整 frame，不要一开始解析 SPI byte 流。

### 3. Communication 通信层

主要路径：

- `source/sources/stratula/library/protocol/`
- `source/sources/stratula/library/protocol/serial/`
- `source/sources/stratula/library/protocol/requests/`
- `source/sources/stratula/library/protocol/commands/`
- `source/targets/atmel/subprojects/RadarBaseboardMCU7/BoardDefinition.h`

主要功能：

- USB CDC vendor protocol。
- 上位机请求解析、响应打包、CRC 校验。
- `RequestHandler` 将上位机命令路由到 GPIO、SPI、I2C、DATA、Radar component 等接口。
- `ProtocolHandler_sendDataFrame()` 将雷达 frame 打包为上位机可解析的数据帧。

分层规则：

- Communication 层只负责传输和协议解析。
- Communication 层不要直接决定雷达寄存器参数。
- Communication 层不要承载算法逻辑。
- 同一个 USB CDC 流不要同时混发 vendor protocol 和普通文本，否则上位机和串口助手都会被污染。

### 4. Actual Firmware / Application 实际固件层

主要路径：

- `source/targets/atmel/subprojects/RadarBaseboardMCU7/main.c`
- `source/targets/atmel/subprojects/RadarBaseboardMCU7/Board.c`
- 后续可新增：`source/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarProcessing.c`
- 后续可新增：`source/targets/atmel/subprojects/RadarBaseboardMCU7/AppRadarProcessing.h`

主要功能：

- 固件启动入口和主循环调度。
- Board 类型、shield、radar 检测。
- 调用驱动层初始化和启动采集。
- 注册数据回调。
- 决定当前输出模式：上位机协议、调试文本、算法结果。
- 安排二次开发算法位置。

分层规则：

- 二次开发主逻辑优先放在这一层。
- 算法入口优先放在完整 frame payload 位置，例如 `Board_dataCallback()` 或独立应用层模块。
- 不要把算法塞进 HAL、SPI transfer、IRQ、DMA completion 回调中。

## Second-Development Roadmap / 二次开发路线

### 第一步：保证 USB CDC 调试输出可控

当前固件的“串口”实际是 USB CDC 虚拟串口，不是传统 UART 引脚串口。

原固件在 USB CDC 上运行 Infineon vendor protocol。雷达原始帧通过 `ProtocolHandler_sendDataFrame()` 加上 header、packet type、counter、CRC 等协议字段后发给上位机。普通串口助手直接观察这一路数据，通常会看到二进制内容或乱码。

二次开发第一步不是删除协议层，而是先建立一个普通调试输出路径：

- 底层输出函数在 `target_platform/impl/serial/SerialPortImpl.c`。
- 可使用 `SerialPort_sendString()` 输出字符串。
- 可使用 `SerialPort_send()` 输出自定义字节数据。
- 初期建议使用 ASCII/CSV，方便串口助手直接观察。

推荐验证顺序：

1. 在 `Board_Constructor()` 初始化完成后输出启动信息，例如 `boot ok`、`radar detected`。
2. 在 `Board_run()` 中低频输出心跳或状态，确认不会阻塞主循环。
3. 在雷达检测成功后输出设备类型、数据 index、采集状态。
4. 在 `Board_dataCallback()` 或后续算法结果位置输出 frame 计数、timestamp、payload size。
5. 稳定后再决定是否输出二进制结果帧。

注意：

- 不要在高帧率下每帧输出大量原始数据，否则 USB CDC 阻塞会影响采集。
- 调试文本只用于验证和算法结果摘要，不建议长期输出完整雷达原始帧。

### 第二步：保留通信层但做模式切换

当前建议保留原通信层，方便后续继续使用 Infineon 上位机或回退调试。

推荐把 USB CDC 输出分成三种模式：

- `HOST_PROTOCOL`：原上位机协议模式，使用 `ProtocolHandler_run()` 和 `ProtocolHandler_sendDataFrame()`。
- `DEBUG_TEXT`：普通文本调试模式，使用 `SerialPort_sendString()` 输出状态、寄存器值、frame 摘要。
- `ALGORITHM_OUTPUT`：算法结果输出模式，使用 CSV 或自定义轻量帧输出结果。

关键规则：

- 同一个 USB CDC 端口不要同时混发 vendor protocol 和普通文本。
- 上位机模式下不要插入普通字符串，否则上位机协议解析可能失败。
- 调试文本模式下可以保留通信层代码，但不要调用原始雷达 frame 的 vendor 打包发送。
- 模式切换可以先用编译宏或全局枚举实现，后续再考虑通过按键、固定配置或简单命令切换。

推荐数据出口设计：

```text
BGT60 -> SPI -> DataAvian -> Board_dataCallback()
                                      |
                                      |-- HOST_PROTOCOL: ProtocolHandler_sendDataFrame()
                                      |
                                      |-- DEBUG_TEXT / ALGORITHM_OUTPUT: SerialPort_sendString()
```

### 第三步：固件内配置雷达参数

原固件的采集参数通常由上位机下发，典型路径是：

```text
host request
-> ProtocolHandlerSerial
-> RequestHandler
-> Requests_IData_configure()
-> DataAvian_configure()
-> DataAvian_start()
```

二次开发目标是逐步让固件上电后自主配置雷达，不依赖上位机下发。

重点处理内容：

- chirp 数。
- sample 数。
- RX/TX 配置。
- frame 周期。
- FIFO readout address。
- readout count。
- `Raw16` / `Packed12` 数据格式。
- `DataAvian_configure()` 所需的 `IDataProperties_t` 和 readout settings。
- `DataAvian_start()` 或 `Avian_startData()` 的启动时机。

推荐做法：

- 把默认雷达参数集中放在 `Board.c` 附近，或新增应用配置文件。
- 不要把参数散落硬编码在 SPI transfer、IRQ handler 或 HAL 函数中。
- 先复用已有 `DataAvian_configure()` 和 Avian register 写入路径，再逐步抽象出更清晰的应用配置函数。
- 每次改寄存器配置后，用调试串口输出关键配置值和启动状态。

### 第四步：在每帧雷达数据接口上放算法

算法入口优先选择完整 frame，而不是 SPI 底层 byte 流，也不是 vendor protocol 打包后的数据。

当前最合适的位置是：

- `Board_dataCallback(void *arg, uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp)`

这里的 `payload` 已经是从 BGT60 FIFO/SPI 读出并由 `DataAvian` 拼好的 frame payload，不包含上位机 vendor protocol header/CRC。

推荐处理方式：

- 早期可以直接在 `Board_dataCallback()` 中统计 frame 数、payload size、timestamp。
- 算法稍复杂时，新增 `AppRadarProcessing.c/.h`。
- `AppRadarProcessing_process(payload, count, channel, timestamp)` 接收完整 frame。
- 算法结果通过 `DEBUG_TEXT` 或 `ALGORITHM_OUTPUT` 模式输出。
- 如果需要保留上位机原始帧上传，则只在 `HOST_PROTOCOL` 模式下调用 `ProtocolHandler_sendDataFrame()`。

禁止做法：

- 不要在 `PlatformSpi_readBurstAsync8()` / `PlatformSpi_readBurstAsync12()` 这类底层读数函数里做算法。
- 不要在 IRQ 回调里做 FFT、滤波、峰值搜索等耗时处理。
- 不要解析已经打包给上位机的 vendor protocol 数据帧来做算法。

## Testing / 测试

文档编码检查：

```powershell
Get-Content .\SKILL.md -Encoding UTF8 -TotalCount 120
```

命令行编译：

```powershell
cd C:\Users\Oliver\Desktop\in\Firmware\source\targets\atmel\subprojects\RadarBaseboardMCU7
mingw32-make BUILD_TYPE=debug
```

Microchip Studio 编译：

```text
Build -> Rebuild Solution
```

验证点：

- Build Output 以 `Build succeeded` 结束。
- `build-debug/RadarBaseboardMCU7.bin` 被重新生成。
- 如果修改数据路径，检查是否引入新的 warning。
- 如果修改 buffer 或算法内存，检查 `build-debug/RadarBaseboardMCU7.map` 中 SRAM/flash 占用。
- 如果只修改 Markdown，不需要编译固件。

## Coding Conventions / 编码习惯

- 保持分层清晰：HAL 不写算法，Driver 不写上位机协议，Communication 不写雷达配置策略。
- 新增业务逻辑优先放在 `Board.c` 附近或新增应用层文件，例如 `AppRadarProcessing.c/.h`。
- 函数命名沿用工程现有风格，例如 `Module_FunctionName()`。
- 修改数据路径时优先处理完整 frame，不直接解析 SPI byte 流。
- 调试输出早期使用 ASCII/CSV，稳定后再考虑二进制帧。
- 不把 `yizhi/` 驱动直接混入主工程，除非明确计划替换 Stratula Avian 路径。
- 不把板级引脚定义写入通用 Stratula 组件。
- 避免在 vendor ASF/CMSIS 代码中做业务修改。
- 改动应小而集中，每次只跨越必要层次。
- 保留原通信层时，新增普通文本输出必须受模式控制。

## Safety / 安全

- 不随意删除原通信层，先通过模式切换保留回退能力。
- 不在 IRQ、DMA completion、SPI transfer 回调中运行耗时算法。
- 不在同一个 USB CDC 流里同时混发 vendor protocol 和普通文本。
- 不硬编码本机工具链路径到共享工程文件；本机路径放 `local_settings.mk`。
- 不大范围改 ASF/CMSIS 厂商代码。
- 烧录前确认生成的 `.bin` 来自当前构建目录。
- 不把雷达寄存器参数分散写在 HAL 或 SPI 底层函数中。
- 如果改动会影响上位机协议兼容性，必须明确当前输出模式和回退方式。

## Example Tasks / 实例任务

- “新增 `DEBUG_TEXT` 模式，用 USB CDC 输出 `radar detected`、frame 计数和时间戳。”
- “在 `Board_dataCallback()` 中接入每帧雷达数据算法入口，但保留 `HOST_PROTOCOL` 模式。”
- “新增 `AppRadarProcessing.c/.h`，输入 frame payload，输出 CSV 格式算法结果。”
- “把 chirp 数、sample 数和 readout settings 从上位机配置改为固件内默认配置函数。”
- “关闭普通文本输出，切回原 `ProtocolHandler_sendDataFrame()` 上位机协议模式。”
- “检查 `DataAvian_configure()` 中 `Raw16` / `Packed12` 对算法输入格式的影响。”
- “用 `SerialPort_sendString()` 输出雷达检测失败原因，便于不用上位机时定位问题。”
- “保留 USB CDC vendor protocol，同时增加编译期开关选择算法结果输出模式。”

