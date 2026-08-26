# Physical AI Demo
## Raspberry Pi側 UART版 仕様書

## 1. 概要

本仕様書は、Physical AI DemoにおけるRaspberry Pi側のUART版ソフトウェア仕様を示す。

Raspberry Pi側はROS2を使用せず、Python単体で構成する。

USBカメラから取得した画像をYOLOv8nで解析し、人物検出結果を安定化したうえで、GPIO UARTを介してM5Stackへ通知する。

---

## 2. システム構成

### 2.1 処理構成

```text
USBカメラ
   ↓
YOLOv8n
   ↓
人物検出
   ↓
安定化
  ├─ 3フレーム連続検出 → ON
  └─ 2秒間未検出      → OFF
   ↓
GPIO UART
   ↓
M5Stack
```

### 2.2 ソフトウェア構成

```text
physical_ai_pi_uart/
├─ main.py
├─ config.py
├─ person_detector.py
├─ m5stack_uart.py
├─ requirements.txt
└─ README.md
```

Python仮想環境をプロジェクト配下に作成する場合は、以下の構成となる。

```text
physical_ai_pi_uart/
├─ .venv/
├─ main.py
├─ config.py
├─ person_detector.py
├─ m5stack_uart.py
├─ requirements.txt
└─ README.md
```

---

## 3. 人物検出仕様

### 3.1 使用モデル

人物検出には以下を使用する。

| 項目 | 設定 |
|---|---|
| モデル | `yolov8n.pt` |
| カメラ番号 | `0` |

UltralyticsのPython APIを使用し、OpenCVで取得したNumPy画像を `predict()` に入力する。  
推論結果からBounding Box（BBox）を取得し、人物の有無を判定する。

### 3.2 検出結果の安定化

YOLOの1フレーム単位の検出結果をそのままM5Stackへ通知せず、以下の条件で状態を安定化する。

| 状態遷移 | 条件 |
|---|---|
| 人物未検出 → 人物検出 | 3フレーム連続で人物を検出 |
| 人物検出 → 人物未検出 | 2秒間連続して人物を検出しない |

人物検出状態が変化した場合のみ、M5Stackへ通知する。

---

## 4. UART通信仕様

### 4.1 通信方式

Raspberry PiとM5Stack間の通信にはGPIO UARTを使用する。

| 項目 | 設定 |
|---|---|
| 通信方式 | UART |
| デバイス | `/dev/serial0` |
| ボーレート | `115200 bps` |

### 4.2 送信データ

#### Raspberry Pi起動時

起動時に現在時刻と人物未検出状態を送信する。

```text
TIME,13:25:30
PERSON,0
```

`TIME` の時刻部分には、Raspberry Piの現在時刻を設定する。

#### 人物検出時

人物検出状態がONへ変化した場合、以下を送信する。

```text
PERSON,1
```

#### 人物未検出時

人物検出状態がOFFへ変化した場合、以下を送信する。

```text
PERSON,0
```

### 4.3 送信タイミング

`PERSON` コマンドは周期送信せず、人物検出状態が変化した場合のみ送信する。

また、Raspberry Pi側プログラムの正常終了時には、

```text
PERSON,0
```

を送信し、M5Stackを時計表示へ戻す。

---

## 5. UARTデバイス確認

Raspberry Pi上で、UARTデバイスの存在を以下のコマンドで確認する。

```bash
ls -l /dev/serial*
```

`/dev/serial0` が存在することを確認したうえで、Raspberry PiとM5StackをGPIO UARTで接続して動作確認を行う。

`raspberrypi-sys-mods` のudevルールが無い環境（Raspberry Pi OS以外のディストリビューションなど）では `/dev/serial0` が作成されない場合がある。
その場合は `ls /dev/tty*` で `ttyAMA0` / `ttyS0` の存在を確認し、`config.py` の `SERIAL_PORT` に直接デバイス名を設定する。

- `/boot/firmware/config.txt` に `dtoverlay=disable-bt` がある場合 → GPIO14/15はフルUART（`/dev/ttyAMA0`）
- `dtoverlay=disable-bt` が無い場合 → GPIO14/15はミニUART（`/dev/ttyS0`、Bluetoothがフル UARTを占有）

---

## 6. Python実行環境

Pythonは仮想環境（venv）上で実行する。

### 6.1 仮想環境作成

```bash
cd physical_ai_pi_uart

python3 -m venv .venv
source .venv/bin/activate
```

### 6.2 Pythonパッケージのインストール

```bash
pip install --upgrade pip
pip install -r requirements.txt
```

---

## 7. 実行方法

通常実行は以下とする。

```bash
python main.py
```

UARTデバイスとカメラ番号を明示して実行する場合は、以下とする。

```bash
python main.py --serial /dev/serial0 --camera 0
```

---

## 8. 終了方法

Python仮想環境を終了する場合は、以下を実行する。

```bash
deactivate
```

Raspberry Pi側プログラムの正常終了時には `PERSON,0` をM5Stackへ送信し、M5Stackを時計表示へ戻す。
