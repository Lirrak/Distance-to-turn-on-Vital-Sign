#ifndef THERMAL_H
#define THERMAL_H

#include <Arduino.h>

void thermalSetupSensor();
void thermalTaskCode(void * pvParameters); 
bool thermalDetectHumanCluster(float &maxTemp);

#endif