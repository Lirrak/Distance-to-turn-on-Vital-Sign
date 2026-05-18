#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include "DFRobot_HumanDetection.h"

// ==========================================
// CẤU HÌNH RADAR KHOẢNG CÁCH (SERIAL 1)
// ==========================================
#define RADAR_RX_PIN 4
#define RADAR_TX_PIN 5
#define RADAR_BAUD 115200 

const float UNIT_RATIO = 2.1; 
const float MAX_VALID_DISTANCE = 200.0; 

const int NUM_READINGS = 2;       
int readings[NUM_READINGS];       
int readIndex = 0;
int total = 0;

// Dùng volatile cho các biến được truy cập chéo giữa 2 nhân (Core 0 & Core 1)
volatile float averageDistance = 0;
volatile bool isPresenceConfirmed = false;

unsigned long lastMotionTime = 0;
const unsigned long OFF_TIMEOUT = 1000; 
String inputBuffer = "";

// ==========================================
// CẤU HÌNH CẢM BIẾN NHỊP TIM/THỞ (SERIAL 2)
// ==========================================
#define VITALS_RX_PIN 16
#define VITALS_TX_PIN 17

DFRobot_HumanDetection hu(&Serial2);

const int numReadingsVitals = 5;
int heartRates[numReadingsVitals] = {0};
int breathRates[numReadingsVitals] = {0};
int readIndexHR = 0, totalHR = 0, averageHR = 0;
int readIndexBR = 0, totalBR = 0, averageBR = 0;

unsigned long vitalsPreviousMillis = 0;
const long vitalsInterval = 1000;

// ==========================================
// CẤU HÌNH CAMERA NHIỆT MLX90640 (I2C)
// ==========================================
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

// ==========================================
// BIẾN GIAO TIẾP ĐA NHÂN (FreeRTOS)
// ==========================================
TaskHandle_t ThermalTask;
volatile float sharedMaxTemp = -100.0;
volatile bool sharedPersonDetected = false;
volatile bool thermalDataReady = false; // Cờ báo hiệu Core 0 đã làm xong việc

// ==========================================
// KHAI BÁO HÀM
// ==========================================
void radarSetup();
void radarHandleData();
void radarCheckTimeout();
void processRadarData(String data);

void thermalSetupSensor();
void thermalTaskCode(void * pvParameters); // Hàm chạy trên Core 0
bool thermalDetectHumanCluster(float &maxTemp);

void vitalsSetup();
void vitalsHandleData();


// ==========================================
// SETUP CHÍNH
// ==========================================
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  radarSetup();
  vitalsSetup();
  thermalSetupSensor();

  // Tạo một Task riêng trên Core 0 để quản lý Camera Nhiệt
  xTaskCreatePinnedToCore(
    thermalTaskCode,   /* Hàm thực thi của Task */
    "ThermalTask",     /* Tên Task */
    10000,             /* Kích thước bộ nhớ Stack */
    NULL,              /* Tham số truyền vào */
    1,                 /* Độ ưu tiên */
    &ThermalTask,      /* Con trỏ quản lý Task */
    0);                /* Ép Task chạy trên Core 0 (0) */
  
  delay(1000);
  Serial.println("\n--- He thong Dual-Core DANG HOAT DONG ---");
}

// ==========================================
// LOOP CHÍNH (Chạy trên Core 1)
// Nhiệm vụ: Đọc UART siêu nhanh & Cập nhật màn hình
// ==========================================
void loop() {
  radarHandleData();          
  radarCheckTimeout();        
  vitalsHandleData();         
  
  // Khi Core 0 báo tín hiệu đã xử lý xong khung ảnh nhiệt mới
  if (thermalDataReady) {
    thermalDataReady = false; // Đóng cờ lại để chờ khung ảnh tiếp theo

    Serial.print("=> [NHIET] Max Temp: ");
    Serial.print(sharedMaxTemp, 2);
    Serial.print(" C | Khoang cach: ");
    Serial.print(averageDistance, 1);
    Serial.print(" cm | Status: ");
    
    if (sharedPersonDetected) Serial.print("[ DETECTED ]");
    else Serial.print("Empty");

    Serial.printf(" | [VITALS] HR: %d bpm - BR: %d nhip/phut\n", averageHR, averageBR);
  }
}

// ==========================================
// MODULE 1: TASK CAMERA NHIỆT (Chạy trên Core 0)
// ==========================================
void thermalTaskCode(void * pvParameters) {
  for(;;) { // Vòng lặp vĩnh cửu của Task
    
    // Chỉ đọc Camera khi Radar báo có người trong cự ly 60cm
    if (isPresenceConfirmed && averageDistance > 0 && averageDistance <= 60.0) {
      
      // getFrame sẽ chặn luồng này, nhưng không làm ảnh hưởng đến Core 1
      if (mlx.getFrame(frame) == 0) {
        
        // Bước 1: Lọc EWMA
        for (int i = 0; i < 768; i++) {
          filteredFrame[i] = (filteredFrame[i] * (1.0 - filterWeight)) + (frame[i] * filterWeight);
        }
        
        // Bước 2: Quét cụm nhiệt
        float tempMax = -100.0;
        bool detected = thermalDetectHumanCluster(tempMax); 

        // Bước 3: Đẩy kết quả ra biến toàn cục & Bật cờ cho Core 1 in ra
        sharedMaxTemp = tempMax;
        sharedPersonDetected = detected;
        thermalDataReady = true; 
      }
    } else {
      // Bắt buộc phải có vTaskDelay để hệ điều hành nhường CPU cho tác vụ khác, tránh lỗi Watchdog
      vTaskDelay(50 / portTICK_PERIOD_MS); 
    }
  }
}

void thermalSetupSensor() {
  Wire.begin(8, 9);
  Wire.setClock(400000); // Hạ từ 1000000 xuống 400000
  
  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("Error: MLX90640 sensor not found!");
    while (1);
  }
  
  mlx.setMode(MLX90640_INTERLEAVED);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_8_HZ); // Nâng tốc độ làm mới lên 8 khung hình/giây
  
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

// ==========================================
// MODULE 2: RADAR
// ==========================================
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

// ==========================================
// MODULE 3: CẢM BIẾN NHỊP TIM/THỞ
// ==========================================
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