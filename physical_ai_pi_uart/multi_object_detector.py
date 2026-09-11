from dataclasses import dataclass

from ultralytics import YOLO


@dataclass
class MultiObjectDetectionResult:
    detected_labels: set
    annotated_frame: object


class MultiObjectDetector:
    def __init__(
        self,
        model_path: str,
        confidence: float,
        class_ids: dict,
        imgsz: int = 640,
    ):
        self._model = YOLO(model_path)
        self._confidence = confidence
        self._imgsz = imgsz
        self._id_to_label = {
            class_id: label for label, class_id in class_ids.items()
        }

    def detect(self, frame) -> MultiObjectDetectionResult:
        results = self._model.predict(
            source=frame,
            conf=self._confidence,
            imgsz=self._imgsz,
            classes=list(self._id_to_label.keys()),
            verbose=False,
        )

        result = results[0]
        boxes = result.boxes

        detected_labels = set()
        if boxes is not None:
            for cls_id in boxes.cls.tolist():
                label = self._id_to_label.get(int(cls_id))
                if label is not None:
                    detected_labels.add(label)

        return MultiObjectDetectionResult(
            detected_labels=detected_labels,
            annotated_frame=result.plot(),
        )
