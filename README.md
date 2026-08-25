# Physical AI Demo

Raspberry PiでUSBカメラの映像をYOLOv8nで解析して人物検出を行い、その結果をGPIO UART経由でM5Stackへ通知するデモです。
M5Stackは普段はアナログ時計を表示し、人物検出時には検出表示に切り替わります。

## システム構成

```text
USBカメラ
   ↓
YOLOv8n (Raspberry Pi)
   ↓
人物検出・安定化
   ↓
GPIO UART
   ↓
M5Stack (時計 / 検出表示)
```

- 人物を3フレーム連続検出でON、最後の検出から2秒間未検出でOFFに切り替える安定化処理を行う
- 状態が変化したときのみUARTで通知する

## リポジトリ構成

```text
physical_ai_ws2/
├─ physical_ai_pi_uart/   # Raspberry Pi側 (Python, YOLOv8n, UART送信)
├─ M5Stack/                # M5Stack側 (PlatformIO, Arduino, M5Unified)
└─ documents/               # 仕様書
```

| ディレクトリ | 内容 |
|---|---|
| [physical_ai_pi_uart/](physical_ai_pi_uart/README.md) | Raspberry Pi側のPythonコードと実行手順 |
| [M5Stack/](M5Stack/README.md) | M5Stack側のPlatformIOプロジェクト |
| [documents/](documents/RaspberryPI側仕様.md) | Raspberry Pi側・M5Stack側の詳細仕様書 |

## UART通信仕様（概要）

| 項目 | 設定 |
|---|---|
| ボーレート | 115200 bps |
| フォーマット | 8N1 |
| 行末 | LF (`\n`) |

| Raspberry Pi | M5Stack Basic |
|---|---|
| GPIO14 / TXD | GPIO16 / RXD2 |
| GPIO15 / RXD | GPIO17 / TXD2 |
| GND | GND |

5Vは接続しない。

### 送信コマンド例

```text
TIME,13:45:30
PERSON,1
PERSON,0
```

詳細は各サブプロジェクトのREADMEおよび[documents/](documents/RaspberryPI側仕様.md)を参照。

## セットアップ概要

### Raspberry Pi側

```bash
cd physical_ai_pi_uart
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
python main.py
```

### M5Stack側

PlatformIOで`M5Stack/`を開いてビルド・書き込みを行う。

- Board: M5Stack Core ESP32
- Framework: Arduino
- Library: M5Unified
