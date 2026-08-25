from dataclasses import dataclass
import time

from ultralytics import YOLO


@dataclass
class DetectionResult:
    person_detected: bool
    person_count: int
    annotated_frame: object


class PersonDetector:
    def __init__(self, model_path: str, confidence: float):
        self._model = YOLO(model_path)
        self._confidence = confidence

    def detect(self, frame) -> DetectionResult:
        results = self._model.predict(
            source=frame,
            conf=self._confidence,
            classes=[0],
            verbose=False,
        )

        result = results[0]
        boxes = result.boxes
        person_count = 0 if boxes is None else len(boxes)

        return DetectionResult(
            person_detected=(person_count > 0),
            person_count=person_count,
            annotated_frame=result.plot(),
        )


class PersonStateFilter:
    def __init__(self, on_consecutive_frames=3, off_delay_sec=2.0):
        self._on_consecutive_frames = on_consecutive_frames
        self._off_delay_sec = off_delay_sec
        self._state = False
        self._consecutive_detected = 0
        self._last_detected_time = None

    def update(self, raw_detected: bool):
        now = time.monotonic()
        changed = False

        if raw_detected:
            self._consecutive_detected += 1
            self._last_detected_time = now

            if (
                not self._state
                and self._consecutive_detected >= self._on_consecutive_frames
            ):
                self._state = True
                changed = True
        else:
            self._consecutive_detected = 0

            if (
                self._state
                and self._last_detected_time is not None
                and (now - self._last_detected_time) >= self._off_delay_sec
            ):
                self._state = False
                changed = True

        return self._state, changed
