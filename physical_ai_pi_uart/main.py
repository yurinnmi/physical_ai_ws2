import argparse
import sys

import cv2

import config
from m5stack_uart import M5StackUart
from multi_object_detector import MultiObjectDetector
from person_detector import PersonDetector, PersonStateFilter


def parse_args():
    parser = argparse.ArgumentParser(
        description="Physical AI demo: YOLO person detection -> M5Stack UART"
    )
    parser.add_argument("--serial", default=config.SERIAL_PORT)
    parser.add_argument("--camera", type=int, default=config.CAMERA_INDEX)
    parser.add_argument("--model", default=config.MODEL_PATH)
    parser.add_argument("--conf", type=float, default=config.CONFIDENCE)
    parser.add_argument("--no-preview", action="store_true")
    return parser.parse_args()


# ============================================================
# 従来の人物検出モード (config.ENABLE_MULTI_OBJECT_DETECTION = False)
# ロジックは変更していない。
# ============================================================
def run_person_detection(args, uart, cap):
    try:
        detector = PersonDetector(args.model, args.conf, config.YOLO_IMGSZ)
    except Exception as exc:
        print(f"ERROR: failed to load YOLO model: {exc}", file=sys.stderr)
        cap.release()
        uart.close()
        return 1

    state_filter = PersonStateFilter(
        config.PERSON_ON_CONSECUTIVE_FRAMES,
        config.PERSON_OFF_DELAY_SEC,
    )

    uart.send_time_now()
    uart.send_person(False)

    show_preview = config.SHOW_PREVIEW and not args.no_preview

    frame_count = 0
    last_result = None
    stable_state = False

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("WARNING: failed to read camera frame.")
                continue

            frame_count += 1
            if frame_count % config.PROCESS_EVERY_N_FRAMES == 0:
                last_result = detector.detect(frame)
                stable_state, changed = state_filter.update(
                    last_result.person_detected
                )

                if changed:
                    uart.send_person(stable_state)

            if show_preview:
                annotated_frame = (
                    last_result.annotated_frame
                    if last_result is not None
                    else frame
                )
                person_count = (
                    last_result.person_count if last_result is not None else 0
                )
                label = (
                    f"PERSON: {'ON' if stable_state else 'OFF'}"
                    f"  count={person_count}"
                )
                cv2.putText(
                    annotated_frame,
                    label,
                    (15, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.8,
                    (255, 255, 255),
                    2,
                    cv2.LINE_AA,
                )
                cv2.imshow(
                    "Physical AI - Person Detection",
                    annotated_frame,
                )

                key = cv2.waitKey(1) & 0xFF
                if key in (ord("q"), 27):
                    break

    except KeyboardInterrupt:
        pass

    finally:
        try:
            uart.send_person(False)
        except Exception:
            pass
        cap.release()
        uart.close()
        cv2.destroyAllWindows()

    return 0


# ============================================================
# 複数物体検出モード (config.ENABLE_MULTI_OBJECT_DETECTION = True)
# teddy bear / cup / bottle。3つとも検出されていれば3つともUART送信する。
# ============================================================
def run_multi_object_detection(args, uart, cap):
    try:
        detector = MultiObjectDetector(
            args.model,
            args.conf,
            config.MULTI_OBJECT_CLASSES,
            config.YOLO_IMGSZ,
        )
    except Exception as exc:
        print(f"ERROR: failed to load YOLO model: {exc}", file=sys.stderr)
        cap.release()
        uart.close()
        return 1

    labels = list(config.MULTI_OBJECT_CLASSES.keys())
    state_filters = {
        label: PersonStateFilter(
            config.PERSON_ON_CONSECUTIVE_FRAMES,
            config.PERSON_OFF_DELAY_SEC,
        )
        for label in labels
    }
    stable_states = {label: False for label in labels}

    uart.send_time_now()
    for label in labels:
        uart.send_object(label, False)

    show_preview = config.SHOW_PREVIEW and not args.no_preview

    frame_count = 0
    last_result = None

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("WARNING: failed to read camera frame.")
                continue

            frame_count += 1
            if frame_count % config.PROCESS_EVERY_N_FRAMES == 0:
                last_result = detector.detect(frame)

                for label in labels:
                    raw_detected = label in last_result.detected_labels
                    stable_states[label], changed = state_filters[label].update(
                        raw_detected
                    )

                    if changed:
                        uart.send_object(label, stable_states[label])

            if show_preview:
                annotated_frame = (
                    last_result.annotated_frame
                    if last_result is not None
                    else frame
                )
                active = [label for label in labels if stable_states[label]]
                label_text = "DETECTED: " + (
                    ", ".join(active) if active else "none"
                )
                cv2.putText(
                    annotated_frame,
                    label_text,
                    (15, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.8,
                    (255, 255, 255),
                    2,
                    cv2.LINE_AA,
                )
                cv2.imshow(
                    "Physical AI - Multi Object Detection",
                    annotated_frame,
                )

                key = cv2.waitKey(1) & 0xFF
                if key in (ord("q"), 27):
                    break

    except KeyboardInterrupt:
        pass

    finally:
        try:
            for label in labels:
                uart.send_object(label, False)
        except Exception:
            pass
        cap.release()
        uart.close()
        cv2.destroyAllWindows()

    return 0


def main():
    args = parse_args()

    print("Physical AI Demo - Raspberry Pi UART")
    print(f"Camera : {args.camera}")
    print(f"Model  : {args.model}")
    print(f"UART   : {args.serial} @ {config.SERIAL_BAUD} bps")
    print(
        "Mode   : "
        + (
            "Multi-object (teddy bear / cup / bottle)"
            if config.ENABLE_MULTI_OBJECT_DETECTION
            else "Person detection"
        )
    )

    try:
        uart = M5StackUart(args.serial, config.SERIAL_BAUD)
    except Exception as exc:
        print(f"ERROR: failed to open UART: {exc}", file=sys.stderr)
        return 1

    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        print("ERROR: failed to open camera.", file=sys.stderr)
        uart.close()
        return 1

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, config.FRAME_WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, config.FRAME_HEIGHT)

    if config.ENABLE_MULTI_OBJECT_DETECTION:
        return run_multi_object_detection(args, uart, cap)

    return run_person_detection(args, uart, cap)


if __name__ == "__main__":
    raise SystemExit(main())
