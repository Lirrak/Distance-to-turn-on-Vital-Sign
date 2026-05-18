#include "SerialAPI.h"
#include "SharedVars.h"

void serialApiSetup() {
  // Cổng Serial đã được khởi tạo ở main.ino với tốc độ 115200
  // Chỉ in ra dòng thông báo để biết hệ thống đã sẵn sàng
  Serial.println("=> [SERIAL API] Da san sang. Gui lenh 'GET_DATA' de lay JSON.");
}

void serialApiHandle() {
  // Kiểm tra xem có dữ liệu gửi từ máy tính xuống không
  if (Serial.available() > 0) {
    // Đọc dữ liệu cho đến khi gặp ký tự xuống dòng (Enter)
    String command = Serial.readStringUntil('\n');
    command.trim(); // Xóa các khoảng trắng hoặc ký tự thừa (\r)

    // Nếu lệnh khớp với "GET_DATA"
    if (command == "GET_DATA") {
      // Đóng gói dữ liệu thành chuẩn JSON
      String json = "{";
      json += "\"radar_distance_cm\":" + String(averageDistance, 1) + ",";
      json += "\"radar_presence\":" + String(isPresenceConfirmed ? "true" : "false") + ",";
      json += "\"vitals_heart_rate\":" + String(averageHR) + ",";
      json += "\"vitals_breath_rate\":" + String(averageBR) + ",";
      json += "\"thermal_max_temp\":" + String(sharedMaxTemp, 2) + ",";
      json += "\"thermal_person_detected\":" + String(sharedPersonDetected ? "true" : "false");
      json += "}";

      // In chuỗi JSON lên Serial để máy tính/phần mềm khác đọc được
      Serial.println(json);
    }
  }
}