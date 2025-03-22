# Download YOLOv8 object detection (Yolo-Weights) form github of ultralytics 
# use python version --->=3.11

from ultralytics import YOLO
import cv2
import math
import time

#url='http://192.168.220.3:81/stream' 
# change url with your esp32 cam module url 
cap = cv2.VideoCapture(0)
if not cap.isOpened(): # if camera not open print
    print("Error: Could not open video stream.")

cap.set(3, 640)  # Reduce width
cap.set(4, 480)  # Reduce height
# cap = cv2.VideoCapture("../Videos/motorbikes.mp4")  # For Video
 
#using YOLO model=yolo v8 large vesrion
model = YOLO("../Yolo-Weights/yolov8l.pt")

#all classes or object it can detect 
classNames = ["person", "bicycle", "car", "motorbike", "aeroplane", "bus", "train", "truck", "boat",
              "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
              "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella",
              "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat",
              "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
              "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli",
              "carrot", "hot dog", "pizza", "donut", "cake", "chair", "sofa", "pottedplant", "bed",
              "diningtable", "toilet", "tvmonitor", "laptop", "mouse", "remote", "keyboard", "cell phone",
              "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors",
              "teddy bear", "hair drier", "toothbrush"
              ]

def initialize_camera():
    cap = cv2.VideoCapture(0)
    cap.set(3, 640)  # Reduce width
    cap.set(4, 480)
    return cap
 
prev_frame_time = 0
new_frame_time = 0
 
while True:
    new_frame_time = time.time()
    success, img = cap.read() #if camera frame is not read by program
    if not success:
        print("Warning: Could not read frame. Reconnecting...")
        cap.release()  # Release the current capture
        time.sleep(2)  # Wait before reconnecting
        cap = initialize_camera()  # Reinitialize the camera
        continue
    
    results = model(img, stream=True)
    for r in results:
        boxes = r.boxes
        for box in boxes:
            # Bounding Box
            x1, y1, x2, y2 = box.xyxy[0]
            x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2)
            cv2.rectangle(img,(x1,y1),(x2,y2),(255,0,255),3)
            # Confidence
            conf = math.ceil((box.conf[0] * 100)) / 100
            # Class Name
            cls = int(box.cls[0])
            # Calculate the position for the text (above the rectangle)
            text_y = max(35, y1 - 10)

            # Put the text on the image 
            cv2.putText(img, f'{classNames[cls]} {conf:.2f}', (max(0, x1), text_y), 
            fontFace=cv2.FONT_HERSHEY_SIMPLEX,  
            fontScale=0.6,  
            color=(255, 0, 255),  
            thickness=2)
 
    fps = 1 / (new_frame_time - prev_frame_time)
    prev_frame_time = new_frame_time
    print(fps)
 
    cv2.imshow("Image", img)
    cv2.waitKey(1)
