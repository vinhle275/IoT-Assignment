#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include "global.h"

LCDState_t determineNextState(SensorData_t currentData);

void executeState(LCDState_t state, SensorData_t data, LiquidCrystal_I2C &lcd, TickType_t &lastBlinkTime, bool &isLcdOn);

void task_lcd(void *pvParameters);


#endif