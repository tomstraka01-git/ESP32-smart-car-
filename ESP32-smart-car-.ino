
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

int defaultSpeed = 128; 



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


void loop() {


  static unsigned long lastRead = 0;
  if (millis() - lastRead >= 500) {
    readBatteryLevel();
    lastRead = millis();
  }


  digitalWrite(BuzzerPin, buzzerON ? HIGH : LOW);
}