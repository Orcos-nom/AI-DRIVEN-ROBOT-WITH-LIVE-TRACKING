import cv2
import time
import math
import random
from flask import Flask, render_template_string, Response
from ultralytics import YOLO

app = Flask(__name__)

# Generate a random local IP address (modify based on your network)
def generate_ip():
    return f"192.168.1.{random.randint(100, 250)}"

# Store the generated IP
esp32_ip = generate_ip()
esp32_stream_url = f"http://192.168.169.3:81/stream"

# Load YOLOv8 Model
model = YOLO("../Yolo-Weights/yolov8l.pt")

# Class Names
classNames = ["person", "bicycle", "car", "motorbike", "aeroplane", "bus", "train", "truck", "boat",
              "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
              "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella",
              "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat",
              "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
              "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli",
              "carrot", "hot dog", "pizza", "donut", "cake", "chair", "sofa", "pottedplant", "bed",
              "diningtable", "toilet", "tvmonitor", "laptop", "mouse", "remote", "keyboard", "cell phone",
              "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors",
              "teddy bear", "hair drier", "toothbrush"]

# HTML Template for Web Page
html_template = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Object Detection</title>
</head>
<body>
    <h1>ESP32-CAM Object Detection</h1>
    <p>ESP32-CAM IP Address: <strong>{{ ip }}</strong></p>
    <img src="{{ url_for('video_feed') }}" width="640" height="480">
</body>
</html>
"""

def generate_frames():
    cap = cv2.VideoCapture(esp32_stream_url)
    if not cap.isOpened():
        print(f"Error: Could not open video stream at {esp32_stream_url}.")
        return

    prev_frame_time = 0

    while True:
        success, img = cap.read()
        if not success:
            print("Warning: Could not read frame. Reconnecting...")
            time.sleep(2)
            cap = cv2.VideoCapture(esp32_stream_url)
            continue

        results = model(img, stream=True)
        for r in results:
            boxes = r.boxes
            for box in boxes:
                # Bounding Box
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                cv2.rectangle(img, (x1, y1), (x2, y2), (255, 0, 255), 3)

                # Confidence
                conf = math.ceil(box.conf[0] * 100) / 100

                # Class Name
                cls = int(box.cls[0])
                label = f"{classNames[cls]} {conf:.2f}"

                # Position for text
                text_y = max(35, y1 - 10)
                cv2.putText(img, label, (max(0, x1), text_y),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 255), 2)

        # Calculate FPS
        new_frame_time = time.time()
        fps = 1 / (new_frame_time - prev_frame_time)
        prev_frame_time = new_frame_time
        print(f"FPS: {fps:.2f}")

        # Convert frame to JPEG
        _, buffer = cv2.imencode(".jpg", img)
        frame = buffer.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/')
def index():
    return render_template_string(html_template, ip=esp32_ip)

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
