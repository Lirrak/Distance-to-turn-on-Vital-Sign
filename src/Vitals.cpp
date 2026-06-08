#include "Vitals.h"
#include "SharedVars.h"
#include "DFRobot_HumanDetection.h"

#define VITALS_RX_PIN 5
#define VITALS_TX_PIN 4

DFRobot_HumanDetection hu(&Serial2);

const int numReadingsVitals = 5;
int heartRates[numReadingsVitals] = {0};
int breathRates[numReadingsVitals] = {0};
int readIndexHR = 0;
int readIndexBR = 0;
int totalHR = 0;
int totalBR = 0;

unsigned long vitalsPreviousMillis = 0;
const unsigned long vitalsInterval = 1000;

void resetVitalsAverages() {
  totalHR = 0;
  totalBR = 0;
  averageHR = 0;
  averageBR = 0;

  for (int i = 0; i < numReadingsVitals; i++) {
    heartRates[i] = 0;
    breathRates[i] = 0;
  }
}

void vitalsSetup() {
  Serial2.begin(115200, SERIAL_8N1, VITALS_RX_PIN, VITALS_TX_PIN);

  vitalsSensorReady = false;

  int retryCount = 0;
  while (hu.begin() != 0 && retryCount < 10) {
    delay(500);
    retryCount++;
  }

  if (retryCount < 10) {
    vitalsSensorReady = true;
  } else {
    vitalsSensorReady = false;
    isPresenceConfirmed = false;
    averageDistance = 0.0f;
    resetVitalsAverages();
  }
}

void vitalsHandleData() {
  if (!vitalsSensorReady) {
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - vitalsPreviousMillis < vitalsInterval) {
    return;
  }
  vitalsPreviousMillis = currentMillis;

  uint16_t presence = hu.smHumanData(hu.eHumanPresence);
  isPresenceConfirmed = (presence == 1);

  if (isPresenceConfirmed) {
    uint16_t currentDistance = hu.smHumanData(hu.eHumanDistance);
    if (currentDistance > 0 && currentDistance < 10000) {
      // Giữ tên biến cm theo code gốc. Bạn nên kiểm chứng lại đơn vị thực tế của sensor.
      averageDistance = (float)currentDistance;
    }

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
    averageDistance = 0.0f;
    resetVitalsAverages();
  }
}
