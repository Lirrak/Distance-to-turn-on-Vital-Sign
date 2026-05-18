#ifndef SHARED_VARS_H
#define SHARED_VARS_H

#include <Arduino.h>

// --- Biến từ Radar ---
extern volatile float averageDistance;
extern volatile bool isPresenceConfirmed;

// --- Biến từ Cảm biến sinh tồn (Vitals) ---
extern int averageHR;
extern int averageBR;

// --- Biến từ Camera nhiệt (Thermal) ---
extern volatile float sharedMaxTemp;
extern volatile bool sharedPersonDetected;
extern volatile bool thermalDataReady;

#endif