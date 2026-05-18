#include "Vitals.h"
#include "SharedVars.h"
#include "DFRobot_HumanDetection.h"

#define VITALS_RX_PIN 16
#define VITALS_TX_PIN 17

DFRobot_HumanDetection hu(&Serial2);

const int numReadingsVitals = 5;
int heartRates[numReadingsVitals] = {0};
int breathRates[numReadingsVitals] = {0};
int readIndexHR = 0, totalHR = 0;
int readIndexBR = 0, totalBR = 0;

unsigned long vitalsPreviousMillis = 0;
const long vitalsInterval = 1000;

void vitalsSetup() {
  Serial2.begin(115200, SERIAL_8N1, VITALS_RX_PIN, VITALS_TX_PIN);
  Serial.println("Initializing DFRobot Vitals Sensor...");
  int retryCount = 0;
  while (hu.begin() != 0 && retryCount < 10) {
    delay(1000);
    retryCount++;
    Serial.print(".");
  }
  if (retryCount >= 10) Serial.println("\nLoi: Khong tim thay cam bien Nhip Tim/Tho!");
  else Serial.println("\nCam bien Nhip Tim/Tho da san sang.");
}

void vitalsHandleData() {
  unsigned long currentMillis = millis();
  if (currentMillis - vitalsPreviousMillis < vitalsInterval) return;
  vitalsPreviousMillis = currentMillis;

  if (hu.smHumanData(hu.eHumanPresence) == 1) {
    int currentHR = hu.getHeartRate();
    int currentBR = hu.getBreatheValue();

    if (currentHR > 30 && currentHR < 200) {
      totalHR -= heartRates[readIndexHR];
      heartRates[readIndexHR] = currentHR;
      totalHR += heartRates[readIndexHR];
      readIndexHR = (readIndexHR + 1) % numReadingsVitals;
      averageHR = totalHR / numReadingsVitals;
    }

    if (currentBR > 5 && currentBR < 60) {
      totalBR -= breathRates[readIndexBR];
      breathRates[readIndexBR] = currentBR;
      totalBR += breathRates[readIndexBR];
      readIndexBR = (readIndexBR + 1) % numReadingsVitals;
      averageBR = totalBR / numReadingsVitals;
    }
  } else {
    totalHR = 0; totalBR = 0;
    averageHR = 0; averageBR = 0;
    for (int i = 0; i < numReadingsVitals; i++) {
      heartRates[i] = 0; breathRates[i] = 0;
    }
  }
}