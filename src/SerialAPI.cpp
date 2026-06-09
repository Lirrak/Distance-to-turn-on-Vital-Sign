#include "SerialAPI.h"
#include "SharedVars.h"

void serialApiSetup() {
  // Không in debug text ra Serial ở đây vì Serial đang được dùng làm API cho IQ9.
  // Giảm timeout để readStringUntil('\n') không block loop quá lâu nếu gói lệnh bị thiếu newline.
  Serial.setTimeout(50);
}

void serialApiSendData() {
  // JSON sạch: mỗi response là đúng 1 dòng và kết thúc bằng '\n'.
  // Python/IQ9 có thể đọc bằng readline() và json.loads().
  Serial.print("{");

  Serial.print("\"type\":\"sensor_data\",");

  Serial.print("\"radar_distance_cm\":");
  Serial.print(averageDistance, 1);
  Serial.print(",");

  Serial.print("\"radar_presence\":");
  Serial.print(isPresenceConfirmed ? "true" : "false");
  Serial.print(",");

  Serial.print("\"vitals_sensor_ready\":");
  Serial.print(vitalsSensorReady ? "true" : "false");
  Serial.print(",");

  Serial.print("\"vitals_heart_rate\":");
  Serial.print(averageHR);
  Serial.print(",");

  Serial.print("\"vitals_breath_rate\":");
  Serial.print(averageBR);
  Serial.print(",");

  Serial.print("\"thermal_sensor_ready\":");
  Serial.print(thermalSensorReady ? "true" : "false");
  Serial.print(",");

  Serial.print("\"thermal_data_valid\":");
  Serial.print(thermalTempValid ? "true" : "false");
  Serial.print(",");

  Serial.print("\"thermal_max_temp\":");
  if (thermalTempValid) {
    Serial.print(sharedMaxTemp, 2);
  } else {
    Serial.print("null");
  }
  Serial.print(",");

  Serial.print("\"thermal_person_detected\":");
  Serial.print(sharedPersonDetected ? "true" : "false");
  Serial.print(",");

  Serial.print("\"uptime_ms\":");
  Serial.print(millis());

  Serial.println("}");
}

void serialApiHandle() {
  while (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "GET_DATA") {
      serialApiSendData();
    }
  }
}
