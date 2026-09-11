CAMERA_INDEX = 0
FRAME_WIDTH = 320
FRAME_HEIGHT = 240

MODEL_PATH = "yolov8n.pt"
CONFIDENCE = 0.50
YOLO_IMGSZ = 320  # YOLO推論時の入力サイズ。小さいほど軽くなるが検出精度は下がる

PROCESS_EVERY_N_FRAMES = 2  # Nフレームに1回だけYOLO推論する。大きいほど軽くなるが検出の反応が遅れる

PERSON_ON_CONSECUTIVE_FRAMES = 3
PERSON_OFF_DELAY_SEC = 2.0

SERIAL_PORT = "/dev/ttyAMA0"
SERIAL_BAUD = 115200

SHOW_PREVIEW = True

# ============================================================
# 複数物体検出 (teddy bear / cup / bottle)
#
# True  : main.py は複数物体検出モードで動作する (multi_object_detector.py)
# False : main.py は従来の人物検出モードで動作する (person_detector.py、変更なし)
# ============================================================
ENABLE_MULTI_OBJECT_DETECTION = True

# label -> yolov8n.pt (COCO学習済み) のクラスID
MULTI_OBJECT_CLASSES = {
    "teddy bear": 77,
    "cup": 41,
    "bottle": 39,
}
