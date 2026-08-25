from datetime import datetime
import serial


class M5StackUart:
    def __init__(self, port: str, baudrate: int = 115200):
        self._serial = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1,
            write_timeout=1.0,
        )

    def close(self):
        if self._serial.is_open:
            self._serial.close()

    def _send_line(self, line: str):
        self._serial.write((line + "\n").encode("ascii"))
        self._serial.flush()
        print(f"UART -> {line}")

    def send_time_now(self):
        self._send_line(datetime.now().strftime("TIME,%H:%M:%S"))

    def send_person(self, detected: bool):
        self._send_line(f"PERSON,{1 if detected else 0}")
