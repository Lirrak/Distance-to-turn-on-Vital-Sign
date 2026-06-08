#include "SharedVars.h"

// Khởi tạo giá trị ban đầu cho các biến toàn cục
volatile float averageDistance = 0.0f;
volatile bool isPresenceConfirmed = false;

volatile bool vitalsSensorReady = false;
int averageHR = 0;
int averageBR = 0;

volatile bool thermalSensorReady = false;
volatile float sharedMaxTemp = -100.0f;
volatile bool sharedPersonDetected = false;
volatile bool thermalDataReady = false;
