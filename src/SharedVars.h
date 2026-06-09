#ifndef SHARED_VARS_H
#define SHARED_VARS_H

#include <Arduino.h>

// --- Biến từ radar / human detection sensor ---
extern volatile float averageDistance;
extern volatile bool isPresenceConfirmed;

// --- Biến trạng thái cảm biến sinh hiệu ---
extern volatile bool vitalsSensorReady;
extern int averageHR;
extern int averageBR;

// --- Biến từ camera nhiệt ---
extern volatile bool thermalSensorReady;
extern volatile float sharedMaxTemp;
extern volatile bool sharedPersonDetected;
extern volatile bool thermalDataReady;
extern volatile bool thermalTempValid;

#endif
