from ultralytics import YOLO
import cv2
import time
import serial

model = YOLO("yolov8n_ncnn_model")
cap = cv2.VideoCapture(0)

frame_id = 0
frame_skip = 2
locked_box = None

prev_yolo_time = time.time()
yolo_fps = 0
ser = serial.Serial(
    '/dev/serial0',  
    baudrate=115200,
    timeout=1
)

def send_command(command, speed):
   
    if not (0 <= command <= 4):
        raise ValueError("Command must be 0-4")
    if not (0 <= speed <= 255):
        raise ValueError("Speed must be 0-255")
    
    ser.write(bytes([command, speed]))
    print(f"Sent: Command={command}, Speed={speed}")

def map_pwm(speed, min_pwm=150, max_pwm=255):
    if speed == 0:
        return 0
    return int(min_pwm + (speed / 100) * (max_pwm - min_pwm))

def calculateSpeed(cx, cy, mx, my):
    dx = cx - mx
    dy = cy - my
    
    if abs(dx) < 20:
        dirX = 0    # stop / center
    elif dx < 0:
        dirX = 3      # left
    else:
        dirX = 4      # right

      
        
    
 
    if dy < 0:
        dirY = "up"
    elif dy > 0:
        dirY = "down"
    else:
        dirY = "center"
    
    speedX = min(abs(dx) / mx * 100, 100) 
    speedY = min(abs(dy) / my * 100, 100) 

    
    return int(speedX), int(speedY), dirX, dirY


while True:
    ret, frame = cap.read()
    if not ret:
        break

    h, w = frame.shape[:2]
    mx = w // 2
    my = h // 2

    cv2.circle(frame, (mx, my), 5, (0, 0, 255), -1)
    if ser.in_waiting > 0:
        battery = ser.read()
        print("Battery Percent:", battery[0])
    # --- YOLO only every N frames ---
    if frame_id % frame_skip == 0:
        results = model(frame, classes=[0])

        detected = False
        for r in results:
            boxes = r.boxes
            if boxes is not None and len(boxes) > 0:
                locked_box = boxes[0]
                detected = True
                break

        # YOLO FPS (real)
        now = time.time()
        yolo_fps = 1 / max(now - prev_yolo_time, 1e-6)
        prev_yolo_time = now

        if not detected:
            locked_box = None

    # --- DRAW EVERY FRAME ---
    if locked_box is not None:
        x1, y1, x2, y2 = map(int, locked_box.xyxy[0].tolist())
        cx = (x1 + x2) // 2
        cy = (y1 + y2) // 2
        
        speedX, speedY, dirX, dirY = calculateSpeed(cx, cy, mx, my)
        mapped_pwmX = map_pwm(speedX)
        if dirX == 3:
            send_command(3, mapped_pwmX)
        elif dirX == 4:
            send_command(4, mapped_pwmX)
        elif dirX == 0:
            send_command(0, mapped_pwmX)
        
        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.circle(frame, (cx, cy), 5, (0, 255, 0), -1)
        cv2.line(frame, (cx, cy), (mx, my), (0, 255, 0), 2)

        cv2.putText(frame, f"SpeedX: {speedX} ({dirX})",
            (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)

        cv2.putText(frame, f"SpeedY: {speedY} ({dirY})",
            (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)
        cv2.putText(frame, f"PwmX: {mapped_pwmX}",
            (10, 120), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)


    cv2.putText(frame, f"YOLO FPS: {yolo_fps:.1f}",
                (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

    cv2.imshow("YOLO on Raspberry Pi", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

    frame_id += 1

cap.release()
cv2.destroyAllWindows()
