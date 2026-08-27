# Physical AI Demo - Raspberry Pi UART

USBカメラをYOLOで解析し、人物検出状態をGPIO UARTでM5Stackへ通知します。

## Files

- `main.py`: 全体制御
- `person_detector.py`: YOLO人物検出と安定化
- `m5stack_uart.py`: M5Stack UART送信
- `config.py`: 設定値
- `requirements.txt`: Python依存

## config.py 設定項目

`config.py` を編集することで、カメラ・検出・UARTの挙動を調整できます。

| 項目 | デフォルト | 説明 |
|---|---|---|
| `CAMERA_INDEX` | `0` | 使用するカメラの`/dev/videoN`番号 |
| `FRAME_WIDTH` / `FRAME_HEIGHT` | `320` / `240` | カメラのキャプチャ解像度。大きいほど画質は上がるが、キャプチャ・プレビュー描画の負荷が増える |
| `MODEL_PATH` | `"yolov8n.pt"` | 使用するYOLOモデルファイル |
| `CONFIDENCE` | `0.50` | 人物と判定する信頼度のしきい値。上げると誤検出は減るが、未検出が増える |
| `YOLO_IMGSZ` | `320` | YOLO推論時に画像を縮小してから入力するサイズ。`FRAME_WIDTH/HEIGHT`とは別に指定でき、小さいほど推論が軽くなるが検出精度は下がる |
| `PROCESS_EVERY_N_FRAMES` | `2` | 何フレームに1回YOLO推論を行うか。大きくするほど負荷は下がるが、状態変化への反応が遅れる（間引いたフレームは直前の検出結果をプレビュー表示に流用する） |
| `PERSON_ON_CONSECUTIVE_FRAMES` | `3` | 人物未検出→検出への切り替えに必要な連続検出フレーム数 |
| `PERSON_OFF_DELAY_SEC` | `2.0` | 人物検出→未検出への切り替えに必要な、最後の検出からの経過秒数 |
| `SERIAL_PORT` | `"/dev/ttyAMA0"` | M5Stackとの通信に使うシリアルデバイス |
| `SERIAL_BAUD` | `115200` | UARTのボーレート（M5Stack側と一致させる必要がある） |
| `SHOW_PREVIEW` | `True` | カメラ映像のプレビューウィンドウを表示するか（`--no-preview`で実行時に上書き可能） |

### 軽量化したい場合のチューニング例

処理が重い・カクつく場合は、以下の順で調整すると効果的です。

1. `PROCESS_EVERY_N_FRAMES` を `3`〜`4` に増やす（推論回数そのものを減らす。効果大）
2. `YOLO_IMGSZ` を `256` など小さくする（1回の推論を軽くする）
3. `FRAME_WIDTH` / `FRAME_HEIGHT` を下げる（キャプチャ・描画の負荷を減らす）
4. `--no-preview` で起動し、プレビュー描画自体を無くす

逆に検出の反応を速くしたい場合は、`PROCESS_EVERY_N_FRAMES` を `1` に戻してください。

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

`requirements.txt` は `--extra-index-url https://download.pytorch.org/whl/cpu` を指定し、CPU専用のtorchを使うようにしています。
Raspberry Pi（NVIDIA GPU非搭載）でCUDA版torchが入ると `Illegal instruction (core dumped)` でクラッシュするため、`torch.__version__` に `+cu***` が付いていないか確認してください。

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

### カメラが起動しない

まず、カメラデバイスが認識されているか確認してください。

```bash
ls -l /dev/video*
```

USBカメラ1台の場合、通常 `/dev/video0` がキャプチャ用、`/dev/video1` はメタデータ用（映像は出ない）というように複数のデバイスが表示されることがあります。

より詳しい情報を見たい場合は `v4l2-ctl` で接続デバイス一覧を確認できます。

```bash
v4l2-ctl --list-devices
```

`command not found` の場合はインストールしてください。

```bash
sudo apt install v4l-utils
```

#### YOLO抜きの最小テストスクリプトでの確認

カメラ番号やGUI表示（プレビューウィンドウ）が正しく機能しているかを、YOLOを使わずに切り分けたい場合は、以下の最小スクリプトで確認できます。

```bash
cd physical_ai_pi_uart
cat > camera_test.py << 'EOF'
import cv2
import sys

camera_index = int(sys.argv[1]) if len(sys.argv) > 1 else 0
cap = cv2.VideoCapture(camera_index)

if not cap.isOpened():
    print(f"ERROR: failed to open camera {camera_index}")
    sys.exit(1)

print(f"Camera {camera_index} opened. Press 'q' to quit.")

while True:
    ok, frame = cap.read()
    if not ok:
        print("WARNING: failed to read frame")
        continue
    cv2.imshow("Camera Test", frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()
EOF
python camera_test.py 0
```

カメラ番号を変えたい場合は引数で指定してください（例: `python camera_test.py 1`）。

映像が表示されれば、そのカメラ番号とGUI表示は問題ありません。表示されない場合は、`main.py`側ではなくカメラ・配線・ディスプレイ環境側の問題に絞り込めます。

確認が終わったら、テスト用スクリプトは削除してください。

```bash
rm camera_test.py
```
