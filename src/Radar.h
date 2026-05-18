#ifndef RADAR_H
#define RADAR_H

#include <Arduino.h>

void radarSetup();
void radarHandleData();
void radarCheckTimeout();
void processRadarData(String data);

#endif