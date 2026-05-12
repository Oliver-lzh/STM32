---
name: radarbaseboard-usb-cdc-output-mode
description: Use when implementing or reviewing the first RadarBaseboardMCU7 secondary-development feature: mutually exclusive USB CDC HOST_PROTOCOL and DEBUG_TEXT output modes without polluting Infineon vendor protocol.
---

# RadarBaseboardMCU7 USB CDC Output Mode Skill

## Purpose

This skill defines the first secondary-development feature for the
RadarBaseboardMCU7 firmware:

- keep the original Infineon host protocol available as `HOST_PROTOCOL`;
- add a normal USB CDC text output mode as `DEBUG_TEXT`;
- make the two modes mutually exclusive on the same USB CDC stream;
- keep the implementation in the application layer, mainly `Board.c`;
- use a repeatable subagent workflow for design, implementation, review, and build verification.

The first feature is not an algorithm feature yet. It creates a safe output
mode framework so later radar processing can output summaries or algorithm
results without breaking the original upper-computer workflow.

## Subagent Workflow

Use this workflow for this feature and for later RadarBaseboardMCU7 secondary
development features.

### Root Controller

The root controller is the only final decision maker.

Responsibilities:

- freeze the exact feature goal and acceptance criteria;
- define the implementation approach and write scope;
- merge subagent outputs into one final skill or patch;
- decide whether the result is acceptable after verification.

The root controller is not a normal subagent.

### Agent A: Implementation Agent

Responsibilities:

- write or propose the concrete code changes;
- touch only the files assigned by the root controller;
- for this feature, primary write scope is `Board.c`;
- do not modify HAL, SPI, IRQ, DMA, protocol CRC, or vendor protocol internals.

### Agent B: Review And Test Agent

Responsibilities:

- check whether the implementation satisfies the feature goal;
- check for protocol pollution, blocking risk, memory risk, and logic risk;
- confirm `HOST_PROTOCOL` remains compatible with the Infineon upper computer;
- confirm `DEBUG_TEXT` does not send vendor protocol frames.

This agent should not write the main feature implementation.

### Agent C: Build Verification Agent

Responsibilities:

- verify the build command, toolchain, output files, and map/size evidence;
- confirm Atmel ARM GCC 6.3.1 is used;
- reject builds that use STM32CubeCLT GCC 13.3.1;
- inspect `.bin`, `.elf`, `.map`, and size output.

This agent does not decide feature design.

### Agent D: Specialist Analysis Agent

For this first feature, Agent D is the USB CDC / vendor protocol specialist.

Responsibilities:

- explain and verify the communication boundary;
- map `SerialPort_sendString()`, `SerialPort_send()`,
  `ProtocolHandler_sendDataFrame()`, and `ProtocolHandlerSerial_sendDataPacket()`;
- confirm the same USB CDC stream never mixes text and vendor protocol bytes.

For later features, this role can change. For example:

- radar configuration specialist for `DataAvian` / `Requests_IData`;
- algorithm specialist for frame payload format and SRAM budget;
- build/release specialist for boot and flash image validation.

## Communication Boundary

RadarBaseboardMCU7 currently uses `COMMUNICATION_SERIAL`, meaning one USB CDC
virtual serial stream carries communication. It is not two independent channels.

The two output paths are:

```text
Normal text path:
Board.c
-> SerialPort_sendString()
-> SerialPort_send()
-> USB CDC

Vendor protocol frame path:
Board_dataCallback()
-> ProtocolHandler_sendDataFrame()
-> ProtocolHandler_sendDataFrameArray()
-> ProtocolHandlerSerial_sendDataPacket()
-> SerialPort_send()
-> USB CDC
```

`SerialPort_sendString()` only wraps `SerialPort_send()` with `strlen()`. It
does not add vendor header, packet type, counter, length, timestamp flag, or CRC.

`ProtocolHandler_sendDataFrame()` is the vendor data-frame entry. In serial mode,
it eventually calls `ProtocolHandlerSerial_sendDataPacket()`, which writes a
binary vendor packet header, payload, optional timestamp, and CRC to the same
USB CDC stream.

Therefore, do not mix normal text with vendor protocol in one run:

- vendor protocol receivers expect binary packet framing and CRC;
- inserting `boot\r\n`, `heartbeat\r\n`, or `frame,...\r\n` breaks framing or CRC;
- normal serial tools show vendor frames as binary/garbled data;
- both paths ultimately use the same `SerialPort_send()` TX buffer.

The isolation point belongs in `Board.c`.

## Target Files

Primary implementation file:

```text
C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\targets\atmel\subprojects\RadarBaseboardMCU7\Board.c
```

Read-only reference files:

```text
C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\targets\atmel\target_platform\impl\serial\SerialPortImpl.c
C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\sources\stratula\library\platform\impl\SerialPortImpl.h
C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\sources\stratula\library\protocol\ProtocolHandlerData.c
C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\sources\stratula\library\protocol\serial\ProtocolHandlerSerial.c
```

Do not modify for this feature:

- `SerialPortImpl.c`;
- `ProtocolHandlerSerial.c`;
- `ProtocolHandlerData.c`;
- `VendorProtocol*`;
- SPI, IRQ, DMA, ASF, CMSIS, HAL, or radar driver internals.

## Output Modes

Use compile-time mode selection first. It is simple, reproducible, and safe for
firmware bring-up.

Recommended mode definitions in `Board.c`:

```c
#define BOARD_USB_CDC_MODE_HOST_PROTOCOL 0u
#define BOARD_USB_CDC_MODE_DEBUG_TEXT    1u

#ifndef BOARD_USB_CDC_OUTPUT_MODE
#define BOARD_USB_CDC_OUTPUT_MODE BOARD_USB_CDC_MODE_HOST_PROTOCOL
#endif
```

Default must be `BOARD_USB_CDC_MODE_HOST_PROTOCOL` so the official upper
computer remains compatible unless the developer intentionally enables text mode.

For local text debugging, change the default or pass a compile flag:

```c
#define BOARD_USB_CDC_OUTPUT_MODE BOARD_USB_CDC_MODE_DEBUG_TEXT
```

Add `ALGORITHM_OUTPUT` only after the text mode is stable.

## Implementation Pattern

### Includes

Only include text-mode dependencies when text mode is enabled:

```c
#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_DEBUG_TEXT
#include <platform/impl/SerialPortImpl.h>
#include <stdio.h>
#endif
```

`SerialPortImpl.h` provides `SerialPort_sendString()` and `SerialPort_send()`.
Use `stdio.h` only for bounded formatting such as `snprintf()`.

### Debug Text Initialization

In `DEBUG_TEXT`, initialize the USB CDC serial path directly. Do this instead of
constructing the full vendor protocol stack.

```c
#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_DEBUG_TEXT
static void Board_debugTextConstructor(void)
{
    SerialPort_Constructor();
    (void)SerialPort_open(115200);
    (void)SerialPort_sendString("boot,mode=DEBUG_TEXT\r\n");
}
#endif
```

The baud rate is effectively placeholder-like for USB CDC, but keep a normal
value for readability.

### Short Text Send Helper

All normal text output must go through one helper:

```c
#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_DEBUG_TEXT
static void Board_debugSendString(const char *message)
{
    (void)SerialPort_sendString(message);
}
#endif
```

Do not make this helper reachable from `HOST_PROTOCOL`.

### Board Constructor

Original code initializes the vendor protocol unconditionally:

```c
ProtocolHandler_Constructor();
RequestHandler_register(gpio, spi, m_data, i2c);
Requests_Macro_register(m_macroBuffer, MACRO_BUFFER_SIZE);
```

Change the communication initialization block to mode-specific behavior:

```c
#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_HOST_PROTOCOL
    ProtocolHandler_Constructor();
    RequestHandler_register(gpio, spi, m_data, i2c);
    Requests_Macro_register(m_macroBuffer, MACRO_BUFFER_SIZE);
#elif BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_DEBUG_TEXT
    Board_debugTextConstructor();
    Board_debugSendString(m_data ? "radar,detected=1\r\n" : "radar,detected=0\r\n");
#else
#error "Unsupported BOARD_USB_CDC_OUTPUT_MODE"
#endif

    if (m_data != NULL)
    {
        m_data->registerCallback(Board_dataCallback, NULL);
    }
```

### Board Run

`ProtocolHandler_run()` must not run in clean text mode, because serial input can
trigger vendor responses and pollute the text stream.

Use:

```c
#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_HOST_PROTOCOL
    ProtocolHandler_run();
#endif
```

Optional low-rate heartbeat for text mode:

```c
#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_DEBUG_TEXT
    static uint32_t heartbeatDivider = 0;
    heartbeatDivider++;
    if (heartbeatDivider >= 100000u)
    {
        heartbeatDivider = 0;
        Board_debugSendString("heartbeat,alive=1\r\n");
    }
#endif
```

Prefer a proper `chrono`-based heartbeat if already adding `chrono` support.
Keep heartbeat low-rate.

### Board Data Callback

Original behavior:

```c
const sr_t ret = ProtocolHandler_sendDataFrame(payload, count, channel, timestamp);
```

Required behavior:

```c
static void Board_dataCallback(void *arg, uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp)
{
    LedSequence_setStatus(LED_STATUS_TRANSFERRING);

#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_HOST_PROTOCOL
    const sr_t ret = ProtocolHandler_sendDataFrame(payload, count, channel, timestamp);
    if (ret != E_SUCCESS)
    {
        m_data->stop(channel);
    }
#elif BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_DEBUG_TEXT
    Board_debugTextSendData(payload, count, channel, timestamp);
#else
#error "Unsupported BOARD_USB_CDC_OUTPUT_MODE"
#endif

    LedSequence_setStatus(LED_STATUS_OPERATING);
}
```

### Frame Summary Output

Only output a short summary. Do not output the full radar payload.

```c
#if BOARD_USB_CDC_OUTPUT_MODE == BOARD_USB_CDC_MODE_DEBUG_TEXT
static uint32_t m_debugFrameCounter = 0;

static void Board_debugTextSendData(const uint8_t *payload, uint32_t count, uint8_t channel, uint64_t timestamp)
{
    char line[96];
    const uint32_t timestampHigh = (uint32_t)(timestamp >> 32);
    const uint32_t timestampLow  = (uint32_t)timestamp;

    m_debugFrameCounter++;
    const int n = snprintf(line,
                           sizeof(line),
                           "frame,%lu,%lu,%u,%lu,%lu,%02X,%02X,%02X,%02X\r\n",
                           (unsigned long)m_debugFrameCounter,
                           (unsigned long)count,
                           channel,
                           (unsigned long)timestampHigh,
                           (unsigned long)timestampLow,
                           count > 0u ? payload[0] : 0u,
                           count > 1u ? payload[1] : 0u,
                           count > 2u ? payload[2] : 0u,
                           count > 3u ? payload[3] : 0u);

    if (n > 0)
    {
        const uint16_t len = (n < (int)sizeof(line)) ? (uint16_t)n : (uint16_t)(sizeof(line) - 1u);
        (void)SerialPort_send((const uint8_t *)line, len);
    }
}
#endif
```

Timestamp is printed as high/low 32-bit fields to avoid depending on `%llu`
formatting. If a single 64-bit timestamp field is required later, verify flash
and stack impact in the `.map` file.

## Review Checklist

- `HOST_PROTOCOL` must not output `boot`, `heartbeat`, `frame,...`, or any other
  ASCII debug text.
- All `SerialPort_sendString()` usage must be unreachable in `HOST_PROTOCOL`.
- `DEBUG_TEXT` must not call `ProtocolHandler_sendDataFrame()`,
  `ProtocolHandler_sendErrorFrame()`, `ProtocolHandler_sendDebugFrame()`, or
  `ProtocolHandler_sendDebugFrameImpl()`.
- `Board_dataCallback()` must split by mode.
- `Board_run()` must not call `ProtocolHandler_run()` in clean `DEBUG_TEXT`.
- `SerialPort_sendString()` and `SerialPort_send()` are blocking-risk points.
  They must not be used in IRQ, DMA completion, or SPI transfer callbacks.
- Frame text output must be short and bounded. Use one line per frame at most,
  and add decimation or time limiting for high-rate data.
- Use `snprintf()` or another bounded formatter. Do not use `sprintf()`.
- Do not use `%f`. Avoid `%llu` unless `.map` growth is checked.
- Do not add large static buffers, frame copies, or `malloc()`.
- Confirm no edits landed in HAL, protocol CRC, vendor protocol, SPI, IRQ, or
  radar driver internals.

## Test Checklist

### Static Checks

Confirm these exist in `Board.c`:

- `BOARD_USB_CDC_MODE_HOST_PROTOCOL`;
- `BOARD_USB_CDC_MODE_DEBUG_TEXT`;
- `BOARD_USB_CDC_OUTPUT_MODE`;
- mode-guarded `ProtocolHandler_run()`;
- mode-guarded `ProtocolHandler_sendDataFrame()`;
- text output only in `DEBUG_TEXT`.

Search examples:

```powershell
Select-String -Path .\Board.c -Pattern "BOARD_USB_CDC_OUTPUT_MODE|ProtocolHandler_run|ProtocolHandler_sendDataFrame|SerialPort_sendString|SerialPort_send"
```

### DEBUG_TEXT Serial Check

After building and flashing a `DEBUG_TEXT` image:

- open a normal serial terminal on the USB CDC COM port;
- expect CRLF-terminated ASCII text;
- examples:

```text
boot,mode=DEBUG_TEXT
radar,detected=1
heartbeat,alive=1
frame,1,8192,0,0,12345678,12,34,56,78
```

No binary vendor frames should appear.

### DEBUG_TEXT Input Pollution Check

While in `DEBUG_TEXT`, type arbitrary characters in the serial terminal.

Expected:

- firmware keeps outputting heartbeat/frame summaries;
- no binary vendor response appears;
- no reset, freeze, or acquisition stop.

### HOST_PROTOCOL Regression Check

After building and flashing a `HOST_PROTOCOL` image:

- Infineon upper computer can connect;
- upper computer can configure/start acquisition;
- frame transfer still works;
- no normal text token appears in the stream.

### Blocking Check

Because `SerialPort_send()` waits for USB CDC TX space and can time out after
about 2000 ms:

- keep text lines short;
- do not print full frame payload;
- if frame rate is high, print every Nth frame or time-limit output;
- if the terminal is closed or not reading, firmware must not permanently hang.

## Build Verification

### Build Directory

```powershell
cd C:\Users\Oliver\Desktop\in\firmware-rbb7\firmware-rbb7\Firmware\sources\targets\atmel\subprojects\RadarBaseboardMCU7
```

### Toolchain Requirement

`Firmware\local_settings.mk` must point to Atmel/Autel Studio ARM GCC:

```makefile
ARM_GCC_PATH := C:/SOFTWA~3/AUTELS~1/7.0/TOOLCH~1/arm/ARM-GN~1/bin/
```

Verify:

```powershell
& "C:\software-3\Autel studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-gcc.exe" --version
```

Expected:

```text
arm-none-eabi-gcc.exe (Atmel build: 508) 6.3.1 20170620
```

Reject builds that show STM32CubeCLT, GNU Tools for STM32, or GCC 13.3.1.

### Build Commands

Prefer explicit build type:

```powershell
mingw32-make BUILD_TYPE=release all
mingw32-make BUILD_TYPE=debug all
```

If using Atmel Studio, ensure both Debug and Release use the external Makefile.

### Output Files

Release outputs:

```text
build-release\RadarBaseboardMCU7.elf
build-release\RadarBaseboardMCU7.bin
build-release\RadarBaseboardMCU7.map
```

Debug outputs:

```text
build-debug\RadarBaseboardMCU7.elf
build-debug\RadarBaseboardMCU7.bin
build-debug\RadarBaseboardMCU7.map
```

Check:

```powershell
Get-Item .\build-release\RadarBaseboardMCU7.elf, .\build-release\RadarBaseboardMCU7.bin, .\build-release\RadarBaseboardMCU7.map
Get-Item .\build-debug\RadarBaseboardMCU7.elf, .\build-debug\RadarBaseboardMCU7.bin, .\build-debug\RadarBaseboardMCU7.map
```

### Size And Map Checks

Use Atmel `size`, not the STM32CubeCLT one:

```powershell
& "C:\software-3\Autel studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-size.exe" -Bx .\build-release\RadarBaseboardMCU7.elf
```

Known baseline from current project state:

```text
Release bin: 54132 bytes
Release text: 54132
Release data: 0
Release bss: 226128

Debug bin: 60612 bytes
Debug text: 60612
Debug data: 0
Debug bss: 226144
```

The linker memory limits are:

```text
rom = 0x00200000
ram = 0x00060000
```

When adding text formatting, compare `.map` growth. Watch especially for:

- unexpected printf formatter growth;
- new `.bss` buffers;
- stack/heap pressure;
- accidental full-frame copies.

## Acceptance Criteria

The feature skill or implementation is acceptable only when:

- default firmware mode remains `HOST_PROTOCOL`;
- `HOST_PROTOCOL` keeps upper-computer compatibility;
- `DEBUG_TEXT` produces ordinary USB CDC text;
- one USB CDC stream never mixes text and vendor protocol;
- `Board.c` owns the mode split;
- HAL, SPI, IRQ, DMA, driver, and protocol internals remain untouched;
- Release and Debug builds pass with Atmel GCC 6.3.1;
- `.bin`, `.elf`, `.map`, and size checks are recorded.

