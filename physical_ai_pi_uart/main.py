import argparse
import sys

import cv2

import config
from m5stack_uart import M5StackUart
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


def main():
    args = parse_args()

    print("Physical AI Demo - Raspberry Pi UART")
    print(f"Camera : {args.camera}")
    print(f"Model  : {args.model}")
    print(f"UART   : {args.serial} @ {config.SERIAL_BAUD} bps")

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

    try:
        detector = PersonDetector(args.model, args.conf)
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

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("WARNING: failed to read camera frame.")
                continue

            result = detector.detect(frame)
            stable_state, changed = state_filter.update(result.person_detected)

            if changed:
                uart.send_person(stable_state)

            if show_preview:
                label = (
                    f"PERSON: {'ON' if stable_state else 'OFF'}"
                    f"  count={result.person_count}"
                )
                cv2.putText(
                    result.annotated_frame,
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
                    result.annotated_frame,
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


if __name__ == "__main__":
    raise SystemExit(main())
