import cv2
import serial
import threading
import requests
import time
import os
from collections import deque

# configuration of details
BOT_TOKEN     = "BOT_TOKEN_HERE"
CHAT_ID       = "CHAT_ID_HERE"

FPS = 20
BUFFER_SECONDS = 2.5
FUTURE_SECONDS = 2.5

SERIAL_PORT = "COM12"
BAUD_RATE = 115200

recording = False

# buffer
buffer_size = int(FPS * BUFFER_SECONDS)
frame_buffer = deque(maxlen=buffer_size)

cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FPS, FPS)

# camera always running
def camera_loop():
    while True:
        ret, frame = cap.read()
        if ret:
            frame_buffer.append(frame)
        time.sleep(1 / FPS)

# record function
def record_and_send():
    global recording

    if recording:
        return

    recording = True
    print("FIRE -> saving buffered video")

    filename = f"fire_{int(time.time())}.mp4"

    if len(frame_buffer) == 0:
        print("no frames")
        recording = False
        return

    h, w = frame_buffer[0].shape[:2]
    out = cv2.VideoWriter(
        filename,
        cv2.VideoWriter_fourcc(*'mp4v'),
        FPS,
        (w, h)
    )

    # OLD frames (2.5 sec)
    for f in list(frame_buffer):
        out.write(f)

    # NEW frames (2.5 sec)
    start = time.time()
    while time.time() - start < FUTURE_SECONDS:
        ret, frame = cap.read()
        if ret:
            out.write(frame)
        time.sleep(1 / FPS)

    out.release()

    send_telegram_message("FIRE DETECTED\nSending video...")

    send_telegram_video(filename)
    os.remove(filename)

    print("Video Successfully sent")

    recording = False


def send_telegram_message(text):
    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"
    requests.post(url, data={"chat_id": CHAT_ID, "text": text})


def send_telegram_video(path):
    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendVideo"
    with open(path, "rb") as f:
        requests.post(url, data={"chat_id": CHAT_ID}, files={"video": f})


# listen ESP32
def listen_serial():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)
    print("Listening to ESP32...")

    while True:
        line = ser.readline().decode().strip()

        if line:
            print("ESP:", line)

        if "FIRE DETECTED" in line:
            if not recording:
                threading.Thread(target=record_and_send, daemon=True).start()


if __name__ == "__main__":
    threading.Thread(target=camera_loop, daemon=True).start()
    listen_serial()