#include "Radar.h"
#include "SharedVars.h"

#define RADAR_RX_PIN 4
#define RADAR_TX_PIN 5
#define RADAR_BAUD 115200 

const float UNIT_RATIO = 2.1; 
const float MAX_VALID_DISTANCE = 200.0; 

const int NUM_READINGS = 2;       
int readings[NUM_READINGS];       
int readIndex = 0;
int total = 0;

unsigned long lastMotionTime = 0;
const unsigned long OFF_TIMEOUT = 1000; 
String inputBuffer = "";

void radarSetup() {
  Serial1.begin(RADAR_BAUD, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  for (int i = 0; i < NUM_READINGS; i++) readings[i] = 0;
}

void radarHandleData() {
  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    if (inChar == '\n') {
      inputBuffer.trim();
      processRadarData(inputBuffer); 
      inputBuffer = ""; 
    } 
    else if (inChar != '\r') {
      inputBuffer += inChar; 
    }
  }
}

void radarCheckTimeout() {
  if (isPresenceConfirmed && (millis() - lastMotionTime > OFF_TIMEOUT)) {
    isPresenceConfirmed = false;
    averageDistance = 0;
    Serial.println("KHONG CO NGUOI (OFF) - Tam dung cac cam bien");
  }
}

void processRadarData(String data) {
  if (data == "ON") {
    lastMotionTime = millis(); 
    if (!isPresenceConfirmed) {
      isPresenceConfirmed = true;
      Serial.println("PHAT HIEN CO NGUOI (ON)");
    }
  } 
  else if (data.startsWith("Range")) {
    int rawValue = data.substring(6).toInt();
    float actualDistanceCM = rawValue * UNIT_RATIO;
    
    if (actualDistanceCM > MAX_VALID_DISTANCE || actualDistanceCM < 10.0) return; 
    
    total = total - readings[readIndex];     
    readings[readIndex] = actualDistanceCM;   
    total = total + readings[readIndex];     
    readIndex = (readIndex + 1) % NUM_READINGS;               
    
    averageDistance = (float)total / NUM_READINGS;
  }
}