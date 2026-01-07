ESP32 Smart Car (WiFi Controlled)

This project is a **WiFi-controlled smart car** using an **ESP32**.
You control the car from your **phone or computer browser**.
It also shows the **battery voltage**, **battery percentage**, and
plays a **buzzer warning when the battery is low**.



---

## ✨ Features

- 📱 Control the car using WiFi (no app needed!)
- 🔋 Live battery voltage and percentage
- ⚠️ Low battery warning with buzzer
- ⬆️⬇️⬅️➡️ Forward, Backward, Left, Right
- 🔌 Rechargeable battery system

---

## 🧠 How It Works (Simple Explanation)

1. The **ESP32 creates its own WiFi network**
2. You connect your phone to that WiFi
3. You open a web page
4. Buttons on the page control the motors
5. The ESP32 checks the battery level
6. If battery is low, the buzzer turns ON

---

## 📡 WiFi Details

| Setting | Value |
|------|------|
| WiFi Name | `ESP32_CAR` |
| Password | `12345678` |
| Website | `192.168.4.1` |

---

## 🔋 Battery Monitoring

- Uses a **voltage divider**
- Reads voltage from ADC pin `GPIO 34`
- Shows:
  - Battery Voltage (V)
  - Battery Percentage (%)
- Buzzer turns ON when battery ≤ **10%**

---

## 🧩 Components Used

- 4× Geared DC motors with wheels
- 2× L298N motor driver modules
- 1× ESP32 Dev Kit
- 1× Breadboard
- 1× Active buzzer
- Jumper wires
- 1× Buck converter (XL4015)
- Plastic car frame
- 1× 24.7kΩ resistor
- 1× 4.7kΩ resistor

---

## 🔌 Pin Connections

### Motor Driver Pins

| ESP32 Pin | Motor Driver |
|--------|-------------|
| 26 | IN1 |
| 27 | IN2 |
| 14 | IN3 |
| 12 | IN4 |

---

### Battery & Buzzer

| Item | ESP32 Pin |
|----|----|
| Battery ADC | GPIO 34 |
| Buzzer | GPIO 13 |

---

## ▶️ How to Run the Project

1. Install **Arduino IDE**
2. Install **ESP32 board support**
3. Open `ESP32_Smart_Car.ino`
4. Select:
   - Board: `ESP32 Dev Module`
   - Correct COM Port
5. Upload the code
6. Connect phone to **ESP32_CAR**
7. Open browser and go to:
