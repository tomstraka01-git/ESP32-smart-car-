from flask import Flask, render_template, Response, jsonify, request
import subprocess
from ultralytics import YOLO
import cv2
import time
import serial
import threading
import sys
import socket
import psutil

ALLOWED_COMMANDS = {
    "shutdown": "sudo shutdown now",
    "reboot": "sudo reboot",
    "update": "sudo apt update && sudo apt upgrade -y",
    

}

app = Flask(__name__)
cap = cv2.VideoCapture(0, cv2.CAP_V4L2)

if not cap.isOpened():
    raise RuntimeError("Cannot open camera")
model = YOLO("yolov8n_ncnn_model")
wanted_area = 100000
area_tolerance = 4000


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
stop_event = threading.Event() 
ai_running = threading.Event()
ai_running.set()

def calculateForwardBackward(current_area, wanted_area, area_tolerance):
    error = wanted_area - current_area
    if abs(error) < area_tolerance:
        return 0, 0
    
    speed = min(abs(error) / wanted_area * 100, 100)
    pwm = map_pwm(speed)

    if error > 0:
        return 1  , pwm   
    else:
        return 2, pwm   


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

def get_cpu_temp():
    with open("/sys/class/thermal/thermal_zone0/temp") as f:
        return round(int(f.read()) / 1000, 1) 

def generate_frames():
    global frame_id, locked_box, prev_yolo_time, yolo_fps

    while not stop_event.is_set():
        if not ai_running.is_set():
           
            import numpy as np
            frame = 255 * np.ones((480, 640, 3), dtype=np.uint8)
            cv2.putText(frame, "AI stopped", (40, 100),
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
            send_command(0, 0)
        else:
           
            frame_id += 1
            ret, frame = cap.read()
            if not ret:
                continue

            h, w = frame.shape[:2]
            mx = w // 2
            my = h // 2
            cv2.circle(frame, (mx, my), 5, (0, 0, 255), -1)

            if ser.in_waiting > 0:
                battery = ser.read()
                print("Battery Percent:", battery[0])

            if frame_id % frame_skip == 0:
                results = model(frame, classes=[0])
                detected = False
                for r in results:
                    boxes = r.boxes
                    if boxes is not None and len(boxes) > 0:
                        locked_box = boxes[0]
                        detected = True
                        break
                now = time.time()
                yolo_fps = 1 / max(now - prev_yolo_time, 1e-6)
                prev_yolo_time = now
                if not detected:
                    locked_box = None

            if locked_box is not None:
                x1, y1, x2, y2 = map(int, locked_box.xyxy[0].tolist())
                current_area = (x2 - x1) * (y2 - y1)
                cx = (x1 + x2) // 2
                cy = (y1 + y2) // 2
                dir_forward_backward, pwmForward_backward = calculateForwardBackward(current_area, wanted_area, area_tolerance)
                speedX, speedY, dirX, dirY = calculateSpeed(cx, cy, mx, my)
                mapped_pwmX = map_pwm(speedX)

                if dirX == 3:
                    send_command(3, mapped_pwmX)
                elif dirX == 4:
                    send_command(4, mapped_pwmX)
                elif dirX == 0:
                    if dir_forward_backward == 1:
                        send_command(1, pwmForward_backward)
                    elif dir_forward_backward == 2:
                        send_command(2, pwmForward_backward)
                    else:
                        send_command(0, 0)

                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.circle(frame, (cx, cy), 5, (0, 255, 0), -1)
                cv2.line(frame, (cx, cy), (mx, my), (0, 255, 0), 2)
                cv2.putText(frame, f"SpeedX: {speedX} ({dirX})", (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)
                cv2.putText(frame, f"SpeedY: {speedY} ({dirY})", (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)
                cv2.putText(frame, f"PwmX: {mapped_pwmX}", (10, 120), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)
                cv2.putText(frame, f"Current area: {current_area}", (10, 150), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)

            cv2.putText(frame, f"YOLO FPS: {yolo_fps:.1f}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        retval, buffer = cv2.imencode('.jpg', frame)
        frame_encoded = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_encoded + b'\r\n')

        
        time.sleep(0.01)
    


received_text = ""  

@app.route("/send_text", methods=["POST"])
def send_text():
    global frame_skip
    data = request.get_json()
    if not data or "text" not in data:
        return jsonify({"status": "error", "message": "No text provided"}), 400

    cmd = data["text"].strip().lower()
    print("Received command:", cmd)

    if cmd in ["quit"]:
        stop_event.set() 

      
        if cap.isOpened():
            cap.release()
        cv2.destroyAllWindows()
        send_command(0, 0)
        if ser.is_open:
            ser.close()

       
        shutdown_func = request.environ.get('werkzeug.server.shutdown')
        if shutdown_func:
            shutdown_func()
        print("Server stopping...")
        subprocess.run("sudo shutdown now", shell=True)
        return jsonify({"status": "ok", "message": "Shutting down"}), 200
        
    elif cmd in ["stop python"]:
        ai_running.clear()
        send_command(0, 0)

        return jsonify({"status": "ok", "message": "Python process stopped"}), 200    
    
    elif cmd.startswith("set frame_skip"):
        parts = cmd.split()

        if len(parts) != 3:
            return jsonify({
            "status": "error",
            "message": "Usage: set frame_skip <number>"
            }), 400

        try:
            value = int(parts[2])
        except ValueError:
            return jsonify({
                "status": "error",
                "message": "Frame skip must be an integer"
            }), 400

        frame_skip = value

        return jsonify({
            "status": "ok",
            "message": f"frame_skip set to {frame_skip}"
        }), 200

        return jsonify({"status": "ok", "message": "Python process stopped"}), 200 
    elif cmd in ["start python"]:
        ai_running.set()
        return jsonify({"status": "ok", "message": "Python process started"}), 200  

    


    elif cmd in ALLOWED_COMMANDS:
        try:
            subprocess.run(ALLOWED_COMMANDS[cmd], shell=True, check=True)
            send_command(0, 0)
            return jsonify({"status": "ok", "command": cmd})
        except subprocess.CalledProcessError as e:
            send_command(0, 0)
            return jsonify({"status": "error", "message": str(e)}), 500

    else:
        return jsonify({"status": "invalid command", "command": cmd}), 403



@app.route("/")
def index():
    return render_template("index.html")

@app.route("/video_feed")
def video_feed():
    return Response(
        generate_frames(),
        mimetype='multipart/x-mixed-replace; boundary=frame'
    )
@app.route("/stats")
def stats():
    temp =  get_cpu_temp()
    load = psutil.cpu_percent()
    return jsonify({"cpu_temp": temp,
                    "cpu_load": load})

if __name__ == "__main__":
    try:
       

        app.run(host="0.0.0.0", port=5000, debug=True, use_reloader=False)
    finally:
        stop_event.set()
        if cap.isOpened():
            cap.release()
        cv2.destroyAllWindows()
        send_command(0, 0)
        if ser.is_open:
            ser.close()
        print("Shutdown done")

