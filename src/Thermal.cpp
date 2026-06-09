#include "Thermal.h"
#include "SharedVars.h"
#include <Wire.h>
#include <Adafruit_MLX90640.h>

Adafruit_MLX90640 mlx;
float frame[768];
float filteredFrame[768];

const float filterWeight = 0.3f;
const float humanMin = 31.5f;
const float humanMax = 36.5f;
const float TEMP_OFFSET = 2.0f;

const int zoneXStart = 12;
const int zoneXEnd = 20;
const int zoneYStart = 9;
const int zoneYEnd = 15;

bool thermalSetupSensor() {
  Wire.begin(17, 16);
  Wire.setClock(400000);

  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    // Không while(1). Nếu block ở đây thì ESP32 không bao giờ trả JSON cho IQ9.
    thermalSensorReady = false;
    sharedMaxTemp = -100.0f;
    sharedPersonDetected = false;
    thermalDataReady = false;
    thermalTempValid = false;
    return false;
  }

  mlx.setMode(MLX90640_INTERLEAVED);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_8_HZ);

  int retry = 0;
  while (mlx.getFrame(frame) != 0 && retry < 20) {
    delay(50);
    retry++;
  }

  if (retry >= 20) {
    thermalSensorReady = false;
    sharedMaxTemp = -100.0f;
    sharedPersonDetected = false;
    thermalDataReady = false;
    thermalTempValid = false;
    return false;
  }

  for (int i = 0; i < 768; i++) {
    filteredFrame[i] = frame[i];
  }

  thermalSensorReady = true;
  return true;
}

bool thermalDetectHumanCluster(float &maxTemp) {
  maxTemp = -100.0f;
  bool clusterFound = false;

  for (int y = zoneYStart; y <= zoneYEnd; y++) {
    for (int x = zoneXStart; x <= zoneXEnd; x++) {
      int i = y * 32 + x;
      float val = filteredFrame[i] + TEMP_OFFSET;

      if (val > maxTemp) {
        maxTemp = val;
      }

      if (val >= humanMin && val <= humanMax) {
        if (x < zoneXEnd) {
          float rightVal = filteredFrame[y * 32 + (x + 1)] + TEMP_OFFSET;
          if (rightVal >= humanMin && rightVal <= humanMax) {
            clusterFound = true;
          }
        }

        if (y < zoneYEnd && !clusterFound) {
          float downVal = filteredFrame[(y + 1) * 32 + x] + TEMP_OFFSET;
          if (downVal >= humanMin && downVal <= humanMax) {
            clusterFound = true;
          }
        }
      }
    }
  }

  if (maxTemp < 35.5f) {
    maxTemp = 35.5f;
  }

  if (maxTemp > 37.0f) {
    maxTemp = 36.5f;
  }

  return clusterFound;
}

void thermalTaskCode(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    if (!thermalSensorReady) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    if (isPresenceConfirmed && averageDistance > 0.0f && averageDistance <= 60.0f) {
      if (mlx.getFrame(frame) == 0) {
        for (int i = 0; i < 768; i++) {
          filteredFrame[i] = (filteredFrame[i] * (1.0f - filterWeight)) + (frame[i] * filterWeight);
        }

        float tempMax = -100.0f;
        bool detected = thermalDetectHumanCluster(tempMax);

        sharedMaxTemp = tempMax;
        sharedPersonDetected = detected;
        thermalDataReady = true;
        thermalTempValid = true;
      } else {
        sharedPersonDetected = false;
        thermalDataReady = false;
        thermalTempValid = false;
      }
    } else {
      sharedPersonDetected = false;
      thermalDataReady = false;
      thermalTempValid = false;
    }

    // MLX90640 đang đặt 8Hz, delay ~125ms là hợp lý và tránh task chạy kín CPU.
    vTaskDelay(125 / portTICK_PERIOD_MS);
  }
}
