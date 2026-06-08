#ifndef THERMAL_H
#define THERMAL_H

#include <Arduino.h>

bool thermalSetupSensor();
void thermalTaskCode(void *pvParameters);
bool thermalDetectHumanCluster(float &maxTemp);

#endif
