#include <Arduino.h>
#include "SharedVars.h"
#include "Radar.h"
#include "Vitals.h"
#include "Thermal.h"
#include "SerialAPI.h" // Gọi thư viện API qua Serial

TaskHandle_t ThermalTask;

void setup() {
  Serial.begin(115200); // Cổng Serial này vừa dùng để debug, vừa làm API
  while (!Serial) delay(10);
  
  radarSetup();
  vitalsSetup();
  thermalSetupSensor();

  serialApiSetup(); // Khởi tạo API

  xTaskCreatePinnedToCore(
    thermalTaskCode,
    "ThermalTask",
    10000,
    NULL,
    1,
    &ThermalTask,
    0); 
}

void loop() {
  radarHandleData();          
  radarCheckTimeout();        
  vitalsHandleData();         
  
  // Lắng nghe lệnh từ cổng USB liên tục
  serialApiHandle();
  
  // LƯU Ý QUAN TRỌNG VỀ DEBUG
  if (thermalDataReady) {
    thermalDataReady = false; 

    // CHÚ Ý: Nếu bạn dùng code Python/NodeJS kết nối vào cổng COM để đọc JSON, 
    // bạn NÊN COMMENT (//) toàn bộ khối Serial.print dưới đây lại để Serial chỉ trả về dữ liệu JSON sạch.
    
    /*
    Serial.print("=> [NHIET] Max Temp: ");
    Serial.print(sharedMaxTemp, 2);
    Serial.print(" C | Khoang cach: ");
    Serial.print(averageDistance, 1);
    Serial.print(" cm | Status: ");
    
    if (sharedPersonDetected) Serial.print("[ DETECTED ]");
    else Serial.print("Empty");

    Serial.printf(" | [VITALS] HR: %d bpm - BR: %d nhip/phut\n", averageHR, averageBR);
    */
  }
}