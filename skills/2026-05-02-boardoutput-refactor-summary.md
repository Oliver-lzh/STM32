# BoardOutput 输出模式说明

## 当前定位

`BoardOutput` 只负责统一管理输出方式：

- 官方上位机协议输出
- 普通 USB CDC 文本输出

现在模式切换入口只有一个地方：`BoardOutput.c` 顶部的 `m_mode`。

## 模式切换

在 `BoardOutput.c` 顶部修改这一行：

```c
static BoardOutput_Mode_t m_mode = BOARD_OUTPUT_MODE_HOST_PROTOCOL;
```

官方上位机协议模式：

```c
static BoardOutput_Mode_t m_mode = BOARD_OUTPUT_MODE_HOST_PROTOCOL;
```

普通文本调试模式：

```c
static BoardOutput_Mode_t m_mode = BOARD_OUTPUT_MODE_DEBUG_TEXT;
```

`Board.c` 不再传入模式参数，也不会覆盖这里的设置。

## 普通文本输出

在 `BOARD_OUTPUT_MODE_DEBUG_TEXT` 模式下，可以在需要的位置手动调用：

```c
BoardOutput_printf("value=%u\r\n", value);
```

注意：

- 不会自动添加换行，需要自己写 `\r\n`。
- 单次格式化缓冲区大小为 160 字节，超过会被截断。
- 在 `BOARD_OUTPUT_MODE_HOST_PROTOCOL` 模式下，`BoardOutput_printf(...)` 不会发送普通文本，避免污染官方上位机协议。

## 函数关系

`Board_Constructor()` 调用：

```c
BoardOutput_Constructor(gpio, spi, m_data, i2c, m_macroBuffer, MACRO_BUFFER_SIZE);
```

`BoardOutput_Constructor()` 根据 `m_mode` 决定初始化哪种输出：

- `HOST_PROTOCOL`：初始化 `ProtocolHandler`、`RequestHandler`、`Requests_Macro`
- `DEBUG_TEXT`：初始化 USB CDC 串口

`Board_run()` 调用：

```c
BoardOutput_run();
```

- `HOST_PROTOCOL`：运行 `ProtocolHandler_run()`
- `DEBUG_TEXT`：不需要轮询协议请求，所以不做额外动作

`Board_dataCallback()` 调用：

```c
BoardOutput_onFrame(payload, count, channel, timestamp);
```

- `HOST_PROTOCOL`：调用 `ProtocolHandler_sendDataFrame(...)` 发送给官方上位机
- `DEBUG_TEXT`：不自动发送雷达帧，直接返回成功

## 编译验证

已使用 Atmel/Autel Studio 自带 ARM GCC 6.3.1 编译验证：

- Release 编译成功
- Debug 编译成功
- `BoardOutput.o` 已重新编译并链接进固件

