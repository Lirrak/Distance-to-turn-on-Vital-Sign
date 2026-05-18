#include "Thermal.h"
#include "SharedVars.h"
#include <Wire.h>
#include <Adafruit_MLX90640.h>

Adafruit_MLX90640 mlx;
float frame[768];
float filteredFrame[768];
float interpolatedFrame[3072]; 

const float filterWeight = 0.3; 
const float humanMin = 31.5;    
const float humanMax = 36.5;    
const float TEMP_OFFSET = 2.0;  

const int zoneXStart = 12; 
const int zoneXEnd = 20;
const int zoneYStart = 9;
const int zoneYEnd = 15;

void thermalSetupSensor() {
  Wire.begin(8, 9);
  Wire.setClock(400000); 
  
  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("Error: MLX90640 sensor not found!");
    while (1);
  }
  
  mlx.setMode(MLX90640_INTERLEAVED);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_8_HZ); 
  
  Serial.println("Reading initial thermal frame...");
  while(mlx.getFrame(frame) != 0) { delay(10); }
  for(int i = 0; i < 768; i++) {
    filteredFrame[i] = frame[i];
  }
  Serial.println("Thermal sensor is ready.");
}

bool thermalDetectHumanCluster(float &maxTemp) {
  maxTemp = -100.0;
  bool clusterFound = false;

  for (int y = zoneYStart; y <= zoneYEnd; y++) {
    for (int x = zoneXStart; x <= zoneXEnd; x++) {
      int i = y * 32 + x;
      float val = filteredFrame[i] + TEMP_OFFSET; 
      
      if (val > maxTemp) maxTemp = val;

      if (val >= humanMin && val <= humanMax) {
        if (x < zoneXEnd) {
          float rightVal = filteredFrame[y * 32 + (x + 1)] + TEMP_OFFSET;
          if (rightVal >= humanMin && rightVal <= humanMax) clusterFound = true;
        }
        if (y < zoneYEnd && !clusterFound) {
          float downVal = filteredFrame[(y + 1) * 32 + x] + TEMP_OFFSET;
          if (downVal >= humanMin && downVal <= humanMax) clusterFound = true;
        }
      }
    }
  }
  return clusterFound;
}

void thermalTaskCode(void * pvParameters) {
  for(;;) { 
    if (isPresenceConfirmed && averageDistance > 0 && averageDistance <= 60.0) {
      if (mlx.getFrame(frame) == 0) {
        
        for (int i = 0; i < 768; i++) {
          filteredFrame[i] = (filteredFrame[i] * (1.0 - filterWeight)) + (frame[i] * filterWeight);
        }
        
        float tempMax = -100.0;
        bool detected = thermalDetectHumanCluster(tempMax); 

        sharedMaxTemp = tempMax;
        sharedPersonDetected = detected;
        thermalDataReady = true; 
      }
    } else {
      vTaskDelay(50 / portTICK_PERIOD_MS); 
    }
  }
}