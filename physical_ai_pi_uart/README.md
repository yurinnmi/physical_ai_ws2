# Physical AI Demo - Raspberry Pi UART

USBカメラをYOLOで解析し、人物検出状態をGPIO UARTでM5Stackへ通知します。

## Files

- `main.py`: 全体制御
- `person_detector.py`: YOLO人物検出と安定化
- `m5stack_uart.py`: M5Stack UART送信
- `config.py`: 設定値
- `requirements.txt`: Python依存

## UART

デフォルト:

- `/dev/serial0`
- 115200 bps
- 8N1

起動時:

```text
TIME,HH:MM:SS
PERSON,0
```

人物を3フレーム連続検出:

```text
PERSON,1
```

最後の人物検出から2秒間未検出:

```text
PERSON,0
```

`PERSON` は状態変化時のみ送信します。

## GPIO wiring

| Raspberry Pi | M5Stack Basic |
|---|---|
| GPIO14 / TXD / Pin 8 | GPIO16 / RXD2 |
| GPIO15 / RXD / Pin 10 | GPIO17 / TXD2 |
| GND | GND |

PiからM5Stackへの一方向通信だけなら、TX→RXとGNDだけでも動作します。
5Vは接続しません。

## 仮想環境

初期設定：  

```bash
cd physical_ai_pi_uart

python3 -m venv .venv
source .venv/bin/activate

pip install --upgrade pip
pip install -r requirements.txt
```

実行：  

```bash
python main.py
```

終了：

```bash
deactivate
```


## Run

```bash
python main.py
```

例:

```bash
python main.py --serial /dev/serial0 --camera 0
```

プレビューなし:

```bash
python main.py --no-preview
```

UART確認:

```bash
ls -l /dev/serial*
```

権限不足の場合:

```bash
sudo usermod -aG dialout $USER
```

実行後、ログアウト・ログインしてください。
