#include "SharedVars.h"

// Khởi tạo giá trị ban đầu cho các biến toàn cục
volatile float averageDistance = 0;
volatile bool isPresenceConfirmed = false;

int averageHR = 0;
int averageBR = 0;

volatile float sharedMaxTemp = -100.0;
volatile bool sharedPersonDetected = false;
volatile bool thermalDataReady = false;