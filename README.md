Dưới đây là mẫu tài liệu mô tả (Description / `README.md`) được viết chuẩn theo văn phong của GitHub. Bạn có thể copy toàn bộ nội dung này và dán trực tiếp vào file `README.md` trên repo của mình.

---

# 🚀 Hệ Thống Phát Hiện Người & Sinh Hiệu Đa Cảm Biến (ESP32-S3 Dual-Core)

Dự án này là một hệ thống nhúng thời gian thực chạy trên mạch **ESP32-S3**, kết hợp sức mạnh của 3 loại cảm biến: Camera nhiệt (MLX90640), Radar đo khoảng cách, và Cảm biến nhịp tim/nhịp thở (DFRobot mmWave).

Hệ thống được thiết kế tối ưu bằng kiến trúc **Dual-Core (FreeRTOS)** để giải quyết triệt để nút thắt cổ chai (bottleneck) giữa giao tiếp I2C chậm và giao tiếp UART tốc độ cao.

## ✨ Tính Năng Nổi Bật

* **⚡ Kiến trúc Đa Nhân (Dual-Core) với FreeRTOS:**
* **Core 0:** Xử lý tác vụ nặng và có tính "chặn" (blocking) của Camera Nhiệt (đọc I2C, tính toán bộ lọc EWMA, nhận diện cụm nhiệt).
* **Core 1:** Chuyên trách quét UART liên tục ở tốc độ cao cho 2 cảm biến Radar và Sinh hiệu, đảm bảo không bị rớt gói tin.


* **🎯 Kích Hoạt Theo Khoảng Cách Thông Minh:** Camera nhiệt và bộ hiển thị sinh hiệu chỉ được kích hoạt hiển thị khi Radar phát hiện có người ở khoảng cách **dưới 60cm**, giúp tiết kiệm tài nguyên và lọc nhiễu môi trường.
* **🔥 Đo Nhiệt Độ Theo Cụm (Cluster Detection):** Thuật toán tự động tìm kiếm và đánh giá các điểm ảnh lân cận trên ma trận 32x24 pixel của MLX90640 để xác nhận thân nhiệt người thật (tránh báo giả).
* **🫀 Giám Sát Sinh Hiệu Chạy Ngầm:** Thu thập và tính toán trung bình (Average Filter) nhịp tim và nhịp thở liên tục ở chế độ nền (background).

## 🛠️ Yêu Cầu Phần Cứng (BOM)

| Linh kiện | Vai trò | Giao tiếp | Chân kết nối (ESP32-S3) |
| --- | --- | --- | --- |
| **ESP32-S3 DevKit** | Vi điều khiển trung tâm | - | - |
| **Adafruit MLX90640** | Camera nhiệt (32x24) | I2C | `SDA: 8`, `SCL: 9` |
| **Radar Khoảng Cách** | Phát hiện có người & Đo cự ly | UART 1 | `RX: 4`, `TX: 5` |
| **DFRobot Human Detection** | Đo nhịp tim & Nhịp thở | UART 2 | `RX: 16`, `TX: 17` |

## ⚙️ Cấu Trúc Hoạt Động (Workflow)

1. **Radar Khoảng Cách** liên tục quét môi trường. Nếu phát hiện chuyển động (`ON`), hệ thống đánh dấu có sự hiện diện.
2. **Cảm biến DFRobot** chạy ngầm liên tục, thu thập dữ liệu nhịp tim (HR) và nhịp thở (BR), sau đó đưa qua bộ lọc trung bình (Moving Average).
3. Khi người tiến vào vùng **< 60cm**:
* Core 0 sẽ lấy khung ảnh nhiệt ở tốc độ 8Hz, lọc nhiễu thông thấp (EWMA) và tìm cụm nhiệt (31.5°C - 36.5°C).
* Core 1 kết hợp khoảng cách, nhiệt độ đỉnh (Max Temp) và dữ liệu sinh hiệu để xuất ra Terminal cùng một lúc.


4. Khi người rời đi (Timeout > 1000ms), hệ thống tự động reset biến trạng thái và ngừng quét camera nhiệt.

## 💻 Cài Đặt & Biên Dịch (PlatformIO)

Dự án này được tối ưu để sử dụng với **PlatformIO IDE** (VS Code).

### 1. Cấu hình `platformio.ini`

Sử dụng cấu hình dưới đây để đảm bảo tốc độ Baudrate cao và tự động nạp code không cần ấn nút cứng trên S3:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

; Tốc độ hiển thị Terminal siêu cao
monitor_speed = 921600

; Tự động Reset sau khi nạp code
upload_resetmethod = nodemcu

; Cấu hình tối ưu cho ESP32-S3 và FreeRTOS
build_flags = 
    -D CORE_DEBUG_LEVEL=1
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

lib_deps =
    adafruit/Adafruit MLX90640 @ ^1.1.1
    adafruit/Adafruit BusIO @ ^1.16.0
    Wire
    SPI

```

### 2. Cài đặt thư viện DFRobot

Thư viện `DFRobot_HumanDetection` cần được cài đặt thủ công.

* Tải mã nguồn từ GitHub của DFRobot.
* Copy thư mục thư viện vào thư mục `lib/` trong project PlatformIO của bạn.

## 📊 Kết Quả Đầu Ra (Serial Monitor)

Khi hệ thống hoạt động ổn định và có người đứng cách < 60cm, Terminal sẽ xuất ra chuỗi dữ liệu thời gian thực dạng:

```text
=> [NHIET] Max Temp: 36.20 C | Khoang cach: 45.5 cm | Status: [ DETECTED ] | [VITALS] HR: 85 bpm - BR: 16 nhip/phut

```

---

*Dự án được xây dựng và phát triển để ứng dụng vào hệ thống giám sát sức khỏe không tiếp xúc / Nhà thông minh.*