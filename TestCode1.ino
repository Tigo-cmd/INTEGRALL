#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define MPU6050_ADDR 0x68

Adafruit_MPU6050 mpu;

// -------- PIN DEFINITIONS --------
#define LED_PIN 25
#define BUTTON_PIN 26
#define BUZZER_PIN 27
#define GSM_RST 5

// -------- GSM SERIAL --------
HardwareSerial sim800(2);

// -------- VARIABLES --------
bool fallAlert = false;
unsigned long fallStartTime = 0;
const int cancelWindow = 10000;

bool buttonHeld = false;
unsigned long buttonHoldStart = 0;

void setup() {

Serial.begin(115200);
delay(1000);

pinMode(LED_PIN, OUTPUT);
pinMode(BUTTON_PIN, INPUT_PULLUP);
pinMode(BUZZER_PIN, OUTPUT);
pinMode(GSM_RST, OUTPUT);

digitalWrite(GSM_RST, HIGH);

// ---------- START I2C ----------
Wire.begin(21,22);

Serial.println("System Booting...");

// ---------- LED BOOT INDICATOR ----------
for(int i=0;i<5;i++){
digitalWrite(LED_PIN, HIGH);
delay(200);
digitalWrite(LED_PIN, LOW);
delay(200);
}

// ---------- WAKE MPU6050 ----------
Serial.println("Waking MPU6050...");

Wire.beginTransmission(MPU6050_ADDR);
Wire.write(0x6B);
Wire.write(0);
Wire.endTransmission(true);

delay(100);

// ---------- INITIALIZE MPU6050 ----------
Serial.println("Initializing MPU6050...");

if(!mpu.begin()){
Serial.println("MPU6050 NOT detected");
while(1);
}

Serial.println("MPU6050 OK");

// Sensor configuration
mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
mpu.setGyroRange(MPU6050_RANGE_500_DEG);
mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

// ---------- START GSM ----------
Serial.println("Starting GSM Module...");

sim800.begin(9600, SERIAL_8N1, 16, 17);

delay(5000); // SIM800L network stabilization

Serial.println("Testing GSM module...");

sendAT("AT");
sendAT("AT+CSQ");
sendAT("AT+CREG?");
sendAT("AT+CMGF=1");

delay(2000);

// ---------- SEND TEST SMS ----------
sendSMS("+2349021919686","ESP32 Fall Detection Device Hardware Test Successful.");

Serial.println("SMS test sent");
}

void loop() {

blinkRandomLED();

readMPU();

checkButton();

handleFallAlert();

}

// -------- RANDOM LED --------
void blinkRandomLED(){

static unsigned long lastBlink=0;

if(millis()-lastBlink>random(500,2000)){
digitalWrite(LED_PIN,!digitalRead(LED_PIN));
lastBlink=millis();
}

}

// -------- MPU TEST --------
void readMPU(){

sensors_event_t a,g,temp;
mpu.getEvent(&a,&g,&temp);

float totalAccel = sqrt(
(a.acceleration.x*a.acceleration.x)+
(a.acceleration.y*a.acceleration.y)+
(a.acceleration.z*a.acceleration.z)
);

Serial.print("ACCEL -> X:");
Serial.print(a.acceleration.x);
Serial.print(" Y:");
Serial.print(a.acceleration.y);
Serial.print(" Z:");
Serial.print(a.acceleration.z);

Serial.print(" | Mag:");
Serial.println(totalAccel);

Serial.print("GYRO -> X:");
Serial.print(g.gyro.x);
Serial.print(" Y:");
Serial.print(g.gyro.y);
Serial.print(" Z:");
Serial.println(g.gyro.z);

Serial.println("--------------------------------");

if(totalAccel > 20 && !fallAlert){
Serial.println("⚠ IMPACT DETECTED!");
triggerFallAlert();
}

delay(300);
}

// -------- FALL ALERT --------
void triggerFallAlert(){

fallAlert=true;
fallStartTime=millis();

tone(BUZZER_PIN,2000);

}

// -------- BUTTON --------
void checkButton(){

if(digitalRead(BUTTON_PIN)==LOW){

if(!buttonHeld){
buttonHeld=true;
buttonHoldStart=millis();
}

if(millis()-buttonHoldStart>3000){

Serial.println("Fall Cancelled");

fallAlert=false;

noTone(BUZZER_PIN);

buttonHeld=false;

}

}else{

buttonHeld=false;

}

}

// -------- FALL TIMER --------
void handleFallAlert(){

if(fallAlert){

if(millis()-fallStartTime>cancelWindow){

Serial.println("Fall Confirmed - Sending SMS");

noTone(BUZZER_PIN);

sendSMS("+2349021919686","ALERT: Possible fall detected. Immediate assistance may be required.");

fallAlert=false;

}

}

}

// -------- AT COMMAND --------
void sendAT(String cmd){

sim800.println(cmd);
delay(1000);

while(sim800.available()){
Serial.write(sim800.read());
}

}

// -------- SEND SMS --------
void sendSMS(String number,String message){

sim800.println("AT+CMGF=1");
delay(1000);

sim800.print("AT+CMGS=\"");
sim800.print(number);
sim800.println("\"");

delay(1000);

sim800.print(message);

delay(500);

sim800.write(26);

delay(5000);

}