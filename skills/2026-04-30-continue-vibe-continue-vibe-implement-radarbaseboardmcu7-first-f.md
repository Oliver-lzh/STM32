# RadarBaseboardMCU7 第一项功能：USB CDC 输出模式切换需求文档

## 摘要
本轮在 RadarBaseboardMCU7 固件中增加 USB CDC 输出模式框架，支持默认官方上位机协议 `HOST_PROTOCOL` 和普通文本调试 `DEBUG_TEXT` 两种编译期模式。

## 目标
默认固件行为必须保持官方兼容：不定义 `BOARD_OUTPUT_MODE` 时继续使用 vendor protocol，上位机通信不被普通文本污染。只有显式编译为 `BOARD_OUTPUT_MODE_DEBUG_TEXT` 时，USB CDC 才输出普通文本调试信息。

## 交付物
- `Board.c` 中的输出模式框架实现。
- 默认 `HOST_PROTOCOL` Release 构建产物与验证证据。
- 显式 `DEBUG_TEXT` 临时构建产物与验证证据。
- `$vibe` 治理记录、专家执行记录、TDD 证据记录和阶段清理记录。

## 约束
- 不修改 HAL、ASF/CMSIS、SPI/DMA 中断路径。
- 不修改协议库或底层 USB CDC 实现。
- 默认模式必须保持官方上位机兼容。
- `HOST_PROTOCOL` 与 `DEBUG_TEXT` 在同一固件中互斥，不能混合输出。
- 调试文本输出不能因为 USB 发送失败而停止雷达采集。
- 后续生成的项目 `.md` 文档默认使用中文。

## 验收标准
- 需求文档在执行前冻结。
- 执行计划在实现前存在。
- 完成声明前存在验证证据。
- 阶段清理记录存在。
- 默认 Release 构建使用 Atmel/Autel Studio 自带 ARM GCC 6.3.1。
- 默认 `HOST_PROTOCOL` 构建成功并生成 `.elf/.hex/.bin`。
- 显式 `DEBUG_TEXT` 构建成功并生成 `.elf/.hex/.bin`。
- `DEBUG_TEXT` 下不调用 `ProtocolHandler_run()` 和 `ProtocolHandler_sendDataFrame()`。
- `DEBUG_TEXT` 下 USB 文本发送失败不调用 `m_data->stop(channel)`。

## 产品验收标准
- 默认固件烧录后应继续兼容官方上位机协议。
- `DEBUG_TEXT` 固件烧录后应能看到类似以下文本：
  `boot,board=RadarBaseboardMCU7`
  `heartbeat,alive=1`
  `frame,<counter>,<size>,<channel>,<timestamp>`
- 真实硬件烧录和上位机通信属于后续人工验证项；本轮完成代码和构建验证。

## TDD 证据状态
本轮 `vibe` 自动冻结了 TDD 证据要求。实际执行中完成了目标构建验证，但没有在实现前记录 failing-first 证据，因此 TDD 证据保持 `manual_review_required`，不做虚假通过声明。

## 非目标
- 不实现雷达算法。
- 不改变官方 vendor protocol 帧格式。
- 不加入运行时命令切换模式。
- 不修改 Atmel Studio 工程结构。

## Runtime 输入真实性
- Governance scope: `root`
- Root run id: `20260430T114600Z-6636e8d4`
- Entry intent: `vibe`
- Requested stop stage: `phase_cleanup`
- Selected pack: `orchestration-core`
- Router-selected skill: `vibe`
- Runtime-selected skill: `vibe`

## 专家/子代理执行
父流程下使用多个 subagent 做了只读审查和验证辅助：
- USB CDC/vendor protocol 边界审查。
- Atmel 构建路径和工具链审查。
- `Board.c` 阻塞风险审查。
- 编译风险静态审查。

## 剩余风险
未做真实硬件烧录、官方上位机连接验证、普通串口文本实机验证。`DEBUG_TEXT` 为避免采集阻塞，会在 CDC TX 缓冲不足时丢弃调试文本。