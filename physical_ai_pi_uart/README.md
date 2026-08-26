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

`/dev/serial0` が存在しない場合、`raspberrypi-sys-mods` のudevルールが無い環境（Raspberry Pi OS以外のディストリビューションなど）である可能性があります。
その場合は `ls /dev/tty*` で `ttyAMA0` / `ttyS0` の存在を確認し、`config.py` の `SERIAL_PORT` または `--serial` オプションで直接デバイス名を指定してください。

- `/boot/firmware/config.txt` に `dtoverlay=disable-bt` がある場合 → GPIO14/15はフルUART（`/dev/ttyAMA0`）
- `dtoverlay=disable-bt` が無い場合 → GPIO14/15はミニUART（`/dev/ttyS0`、Bluetoothがフル UARTを占有）

### Permission denied

`python main.py` 実行時に `/dev/ttyAMA0` (または `/dev/serial0`) で `Permission denied` になる場合、ユーザーが `dialout` グループに入っていないことが原因です。

```bash
groups
```

の結果に `dialout` が含まれているか確認してください。含まれていなければ:

```bash
sudo usermod -aG dialout $USER
sudo reboot
```

を実行してください（グループ反映のためログアウト・ログインだけでも可）。

この設定は**ユーザーアカウントに対する恒久的な設定**のため、一度行えば以降は毎回実行する必要はありません。OSを再インストールしたり、別ユーザーで実行する場合のみ再設定が必要です。

それでも解決しない場合、シリアルコンソールがポートを占有している可能性があります。

```bash
sudo systemctl status serial-getty@ttyAMA0.service
```

が `active (running)` の場合:

```bash
sudo systemctl stop serial-getty@ttyAMA0.service
sudo systemctl disable serial-getty@ttyAMA0.service
```
