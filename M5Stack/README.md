# Physical AI Demo - M5Stack

Raspberry Piで人物検出を行い、その結果をGPIO UARTでM5Stackへ通知するデモです。

## PlatformIO

- Board: M5Stack Core ESP32
- Framework: Arduino
- Library: M5Unified

## Project structure

```text
physical_ai_m5stack_platformio/
├─ .gitignore
├─ platformio.ini
├─ README.md
├─ include/
├─ lib/
├─ src/
│  ├─ main.cpp
│  ├─ clock_display.h
│  └─ clock_display.cpp
└─ test/
```

## UART wiring

| Raspberry Pi | M5Stack Basic |
|---|---|
| GPIO14 / TXD | GPIO16 / RXD2 |
| GPIO15 / RXD | GPIO17 / TXD2 |
| GND | GND |

5Vは接続しません。

## UART settings

- 115200 bps
- 8 data bits
- No parity
- 1 stop bit
- No flow control
- LF (`\n`) line ending

## Commands

起動時に時刻を1回送信:

```text
TIME,13:45:30
```

人物検出時:

```text
PERSON,1
```

人物がいなくなった時:

```text
PERSON,0
```

## Display behavior

通常時:
- アナログ時計
- 起動直後・TIME未受信の状態でも、`_timeValid` のデフォルトが `true` のため 00:00:00 を起点に時計を表示する
  - `clock_display.h` の `_timeValid` を `false` にした場合は、TIME未受信時は `--:--` と表示され、TIMEを一度受信するまで時計は表示されない

人物検出時:
- 黒背景
- 白文字
- 中央に大きく2行表示

```text
PERSON
DETECTED
```
