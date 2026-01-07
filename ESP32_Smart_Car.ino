#include <WiFi.h>
#include <WebServer.h>


const char* ap_ssid = "ESP32_CAR";
const char* ap_password = "12345678";


const int adcPin = 34;
const int BuzzerPin = 13;
bool buzzerON = false;


#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12

WebServer server(80);


float currentBatteryVoltage = 0.0;
int batteryPercent = 0;

const float minBatteryVoltage = 10.0;   
const float maxBatteryVoltage = 12.6;   

// Voltage divider resistors
const float R1 = 24700.0;
const float R2 = 4700.0;
const float dividerFactor = (R1 + R2) / R2;

const int samples = 10;


void readBatteryLevel() {
  uint32_t rawSum = 0;

  for (int i = 0; i < samples; i++) {
    rawSum += analogRead(adcPin);
    delay(2);
  }

  float rawAvg = rawSum / (float)samples;
  float adcVoltage = (rawAvg / 4095.0) * 3.6;
  currentBatteryVoltage = adcVoltage * dividerFactor;

  batteryPercent = (int)((currentBatteryVoltage - minBatteryVoltage) * 100.0 /
                         (maxBatteryVoltage - minBatteryVoltage));

  batteryPercent = constrain(batteryPercent, 0, 100);

  
  buzzerON = (batteryPercent <= 10);

  Serial.printf("Battery: %.2f V | %d %%\n", currentBatteryVoltage, batteryPercent);
}


void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}


void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Robot Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; background:#f2f2f2; }
    button { font-size: 18px; padding: 12px 24px; margin: 5px; }
    #warning { color:red; font-weight:bold; font-size:20px; display:none; }
  </style>
</head>
<body>

<h2>ESP32 Robot Control</h2>

<h3>Battery</h3>
<p>Voltage: <span id="voltage">--</span> V</p>
<p>Charge: <span id="percent">--</span> %</p>
<p id="warning">⚠ LOW BATTERY! PLEASE CHARGE ⚠</p>

<hr>

<button onclick="fetch('/forward')">Forward</button><br><br>
<button onclick="fetch('/left')">Left</button>
<button onclick="fetch('/right')">Right</button><br><br>
<button onclick="fetch('/backward')">Backward</button><br><br>
<button onclick="fetch('/stop')">Stop</button>

<script>
setInterval(() => {
  fetch('/battery')
    .then(r => r.json())
    .then(data => {
      document.getElementById('voltage').innerText = data.voltage.toFixed(2);
      document.getElementById('percent').innerText = data.percent;
      document.getElementById('warning').style.display =
        data.low ? "block" : "none";
    });
}, 500);
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}


void setupServer() {
  server.on("/", handleRoot);

  server.on("/battery", []() {
    String json = "{";
    json += "\"voltage\":" + String(currentBatteryVoltage, 2) + ",";
    json += "\"percent\":" + String(batteryPercent) + ",";
    json += "\"low\":" + String(buzzerON ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/forward", []() { forward(); server.send(200); });
  server.on("/backward", []() { backward(); server.send(200); });
  server.on("/left", []() { left(); server.send(200); });
  server.on("/right", []() { right(); server.send(200); });
  server.on("/stop", []() { stopMotors(); server.send(200); });

  server.begin();
}


void setup() {
  Serial.begin(115200);

  pinMode(BuzzerPin, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  analogReadResolution(12);
  analogSetPinAttenuation(adcPin, ADC_11db);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  setupServer();
}


void loop() {
  server.handleClient();

  static unsigned long lastRead = 0;
  if (millis() - lastRead >= 500) {
    readBatteryLevel();
    lastRead = millis();
  }


  digitalWrite(BuzzerPin, buzzerON ? HIGH : LOW);
}
