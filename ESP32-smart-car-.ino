
const int adcPin = 34;
const int BuzzerPin = 13;
bool buzzerON = false;


#define IN1 26
#define IN2 27
#define IN3 32   
#define IN4 33

#define IN5 19
#define IN6 21
#define IN7 22
#define IN8 23


const int ENA1 = 25;  
const int ENB1 = 16;  

const int ENA2 = 17;   
const int ENB2 = 18;   


const int freq = 1000;     
const int resolution = 8; 

const int M1_CH = 0;
const int M2_CH = 1;
const int M3_CH = 2;
const int M4_CH = 3;

uint8_t currentCommand = 0;

int defaultSpeed = 128; 

// Motor smoothing variables
uint8_t current_speed = 0;
uint8_t speed_step = 5;
uint8_t wanted_speed = 0;
unsigned long lastTime = 0;

// Speed variables

int s1 = current_speed;
int s2 = current_speed;
int s3 = current_speed;
int s4 = current_speed;

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

  

}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  digitalWrite(IN5, LOW);
  digitalWrite(IN6, LOW);
  digitalWrite(IN7, LOW);
  digitalWrite(IN8, LOW);

  ledcWrite(ENA1, 0);
  ledcWrite(ENB1, 0);
  ledcWrite(ENA2, 0);
  ledcWrite(ENB2, 0);
}


void forward(int speedM1, int speedM2, int speedM3, int speedM4) {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

 
  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, LOW);
  digitalWrite(IN7, HIGH);
  digitalWrite(IN8, LOW);

  ledcWrite(ENA1, speedM1);
  ledcWrite(ENB1, speedM2);
  ledcWrite(ENA2, speedM3);
  ledcWrite(ENB2, speedM4);
}


void backward(int speedM1, int speedM2, int speedM3, int speedM4) {

  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

 
  digitalWrite(IN5, LOW);
  digitalWrite(IN6, HIGH);
  digitalWrite(IN7, LOW);
  digitalWrite(IN8, HIGH);

  ledcWrite(ENA1, speedM1);
  ledcWrite(ENB1, speedM2);
  ledcWrite(ENA2, speedM3);
  ledcWrite(ENB2, speedM4);
}


void left(int speedM1, int speedM2, int speedM3, int speedM4) {

  digitalWrite(IN1, HIGH);   
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  // RIGHT side → forward
  digitalWrite(IN5, LOW);
  digitalWrite(IN6, HIGH);
  digitalWrite(IN7, HIGH);
  digitalWrite(IN8, LOW);

  ledcWrite(ENA1, speedM1);
  ledcWrite(ENB1, speedM2);
  ledcWrite(ENA2, speedM3);
  ledcWrite(ENB2, speedM4);
}


void right(int speedM1, int speedM2, int speedM3, int speedM4) {
 
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  
  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, LOW);
  digitalWrite(IN7, LOW);
  digitalWrite(IN8, HIGH);
  
  ledcWrite(ENA1, speedM1);
  ledcWrite(ENB1, speedM2);
  ledcWrite(ENA2, speedM3);
  ledcWrite(ENB2, speedM4);
}






void setup() {
  Serial.begin(115200);

  pinMode(BuzzerPin, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(IN5, OUTPUT);
  pinMode(IN6, OUTPUT);
  pinMode(IN7, OUTPUT);
  pinMode(IN8, OUTPUT);
    
  ledcAttach(ENA1, 1000, 8);
  ledcAttach(ENB1, 1000, 8);
  ledcAttach(ENA2, 1000, 8);
  ledcAttach(ENB2, 1000, 8);



  stopMotors();

  analogReadResolution(12);
  analogSetPinAttenuation(adcPin, ADC_11db);


}

void sendBatterySerial() {
  Serial.print("BAT,");
  Serial.print(currentBatteryVoltage, 2);
  Serial.print(",");
  Serial.println(batteryPercent);
}



void loop() {
  static unsigned long lastBatteryRead = 0;

  // Smooth acceleration every 10 ms
  if (millis() - lastTime >= 10) {
      if (current_speed < wanted_speed)
          current_speed += speed_step;
      else if (current_speed > wanted_speed)
          current_speed -= speed_step;

      current_speed = constrain(current_speed, 0, 255); 
      lastTime = millis();
}

  // Update speed variables for motors
  s1 = current_speed;
  s2 = current_speed;
  s3 = current_speed;
  s4 = current_speed;

  // Read serial commands
  while (Serial.available() >= 2) { 
    uint8_t command = Serial.read();
    uint8_t speed   = Serial.read();
    currentCommand = command;
    wanted_speed   = speed;
  }

  // Execute motors based on current command
  switch (currentCommand) {
    case 0: stopMotors(); break;
    case 1: forward(s1,s2,s3,s4); break;
    case 2: backward(s1,s2,s3,s4); break;
    case 3: left(s1,s2,s3,s4); break;
    case 4: right(s1,s2,s3,s4); break;
    default: stopMotors(); break;
  }

  // Read battery every 500 ms
  if (millis() - lastBatteryRead >= 500) {
    readBatteryLevel();
    sendBatterySerial();
    lastBatteryRead = millis();
  }

  // Update buzzer
  digitalWrite(BuzzerPin, buzzerON ? HIGH : LOW);
}