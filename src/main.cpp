#include <Arduino.h>
#include "SharedVars.h"
#include "Vitals.h"
#include "Thermal.h"
#include "SerialAPI.h"

TaskHandle_t ThermalTask = NULL;

void setup() {
  Serial.begin(115200);

  // Không chờ Serial vô hạn. Trên ESP32-S3 USB CDC, while(!Serial) có thể làm board
  // đứng mãi nếu IQ9/Python mở cổng nhưng không assert DTR như Serial Monitor.
  unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 1500) {
    delay(10);
  }

  // API phải được setup sớm để IQ9 luôn có thể gọi GET_DATA.
  serialApiSetup();

  // Setup cảm biến không được làm ESP32 kẹt vĩnh viễn.
  vitalsSetup();
  thermalSetupSensor();

  xTaskCreatePinnedToCore(
    thermalTaskCode,
    "ThermalTask",
    10000,
    NULL,
    1,
    &ThermalTask,
    0
  );
}

void loop() {
  // Ưu tiên xử lý lệnh từ IQ9 trước để API phản hồi nhanh.
  serialApiHandle();

  vitalsHandleData();

  // Gọi thêm một lần sau khi đọc sensor để nếu IQ9 vừa gửi lệnh thì có phản hồi ngay.
  serialApiHandle();

  if (thermalDataReady) {
    thermalDataReady = false;
  }

  delay(2);
}
