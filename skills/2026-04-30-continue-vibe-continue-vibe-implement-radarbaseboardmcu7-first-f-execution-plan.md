# RadarBaseboardMCU7 第一项功能：USB CDC 输出模式切换执行计划

## 执行摘要
本计划在 `Board.c` 增加编译期 USB CDC 输出模式框架：默认 `HOST_PROTOCOL` 保持官方上位机协议，显式 `DEBUG_TEXT` 用于普通串口文本调试。执行方式采用主控集成者 + 多 subagent 审查/验证。

## 冻结输入
- Requirement doc: `C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\docs\requirements\2026-04-30-continue-vibe-continue-vibe-implement-radarbaseboardmcu7-first-f.md`
- Runtime input packet: `C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\outputs\runtime\vibe-sessions\20260430T114600Z-6636e8d4\runtime-input-packet.json`
- Root run id: `20260430T114600Z-6636e8d4`
- Entry intent: `vibe`
- Governance scope: `root`

## 防目标漂移控制
主要目标是输出模式框架，不扩展到雷达算法、HAL、SPI/DMA、协议库或上位机协议格式变更。

### 主要目标
实现 `BOARD_OUTPUT_MODE_HOST_PROTOCOL` 与 `BOARD_OUTPUT_MODE_DEBUG_TEXT` 两种互斥编译期模式，默认保持 `HOST_PROTOCOL`。

### 非目标代理信号
- 只看到一个样例输出不代表完整完成。
- 只构建通过不代表硬件实机验证完成。
- 只完成流程产物不代表产品验收完成。

## 波次计划
- Wave 1: 读取 `Board.c`、USB CDC、vendor protocol、构建脚本，冻结实现边界。
- Wave 2: 在 `Board.c` 实现模式宏、协议分支、文本输出路径和非阻塞调试输出。
- Wave 3: 执行默认 Release 构建、显式 `DEBUG_TEXT` 构建、subagent 审查、`vibe` 证据刷新。

## 实现步骤
1. 在 `Board.c` 中新增 `BOARD_OUTPUT_MODE_HOST_PROTOCOL`、`BOARD_OUTPUT_MODE_DEBUG_TEXT`、`BOARD_OUTPUT_MODE` 默认值。
2. 默认 `HOST_PROTOCOL` 下保留 `ProtocolHandler_Constructor()`、`RequestHandler_register()`、`Requests_Macro_register()`、`ProtocolHandler_run()`、`ProtocolHandler_sendDataFrame()`。
3. `DEBUG_TEXT` 下初始化 USB CDC 串口并输出 boot/heartbeat/frame 文本。
4. `DEBUG_TEXT` 下禁止调用 vendor protocol 数据帧发送和协议轮询。
5. `Board_dataCallback()` 在 `DEBUG_TEXT` 下只记录待输出帧摘要，不直接阻塞发送。
6. `Board_run()` 在 `DEBUG_TEXT` 下检查 CDC TX ready/free buffer 后再尝试发送，缓冲不足时跳过，避免采集被调试输出卡住。

## 专家/子代理分工
- 实现/主控：修改并集成 `Board.c`。
- 协议审查 subagent：确认 `HOST_PROTOCOL` 与 `DEBUG_TEXT` 不混流。
- 构建审查 subagent：确认 Atmel/Autel Studio GCC 6.3.1 和产物路径。
- 代码风险审查 subagent：检查编译风险、阻塞风险、未使用变量风险。

## 验证命令
默认官方协议构建：

```powershell
& "C:\software-3\Autel studio\7.0\shellUtils\make.exe" -C "C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\targets\atmel\subprojects\RadarBaseboardMCU7" -f "makefile" BUILD_TYPE=release
```

显式 DEBUG_TEXT 构建：

```powershell
& "C:\software-3\Autel studio\7.0\shellUtils\make.exe" -C "C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\targets\atmel\subprojects\RadarBaseboardMCU7" -f "makefile" BUILD_TYPE=release BUILD_DIR=build-debugtext "CFLAGS+=-DBOARD_OUTPUT_MODE=BOARD_OUTPUT_MODE_DEBUG_TEXT"
```

确认工具链：

```powershell
& "C:\software-3\Autel studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-gcc.exe" --version
```

## 产物路径
- 默认协议固件：`sources\targets\atmel\subprojects\RadarBaseboardMCU7\build-release\RadarBaseboardMCU7.bin`
- 默认协议 ELF/MAP：`build-release\RadarBaseboardMCU7.elf`、`build-release\RadarBaseboardMCU7.map`
- DEBUG_TEXT 临时固件：`sources\targets\atmel\subprojects\RadarBaseboardMCU7\build-debugtext\RadarBaseboardMCU7.bin`

## 完成措辞规则
- 工程验证通过可以说明为“构建验证通过”。
- 未做硬件烧录前，不能说实机行为完全验证完成。
- 没有实现前 failing-first 证据，因此 `$vibe` delivery acceptance 保持 TDD manual review，不做虚假 PASS。

## 回滚计划
如果验证失败，只回滚本轮 `Board.c` 的输出模式框架改动，不回滚无关用户文件或构建产物。

## 阶段清理契约
- 记录 `specialist-execution.json`。
- 记录 `tdd-evidence.json`。
- 记录中文 `implementation-summary.md`。
- 刷新 `delivery-acceptance-report.json/md`。