#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TEMP_HOT 35.0
#define TEMP_WARM 30.0
#define TEMP_COOL 25.0
#define TEMP_COLD 20.0

#define HUMIDITY_VERY_HUMID 75.0
#define HUMIDITY_HUMID      60.0
#define HUMIDITY_DRY        40.0
#define HUMIDITY_VERY_DRY   25.0

typedef enum {
    LED_STATE_TEMP_HOT,  
    LED_STATE_TEMP_WARM,  
    LED_STATE_NORMAL_TEMP,                 
    LED_STATE_TEMP_COOL,     
    LED_STATE_TEMP_COLD       
} LEDBlinkyState_t;

typedef enum {
    LED_STATE_HUMIDITY_VERY_HUMID,  
    LED_STATE_HUMIDITY_HUMID,       
    LED_STATE_NORMAL_HUMI,                 
    LED_STATE_HUMIDITY_DRY,         
    LED_STATE_HUMIDITY_VERY_DRY,
} NeoBlinkyState_t;

typedef enum {       
    LCD_STATE_NORMAL,                 
    LCD_STATE_WARNING,
    LCD_STATE_CRITICAL     
} LCDState_t;

typedef struct {
    float temperature;
    float humidity;
} SensorData_t;

extern QueueHandle_t sensorQueue;

// extern String WIFI_SSID;
// extern String WIFI_PASS;
// extern String CORE_IOT_TOKEN;
// extern String CORE_IOT_SERVER;
// extern String CORE_IOT_PORT;



extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

//Task 6
extern SemaphoreHandle_t xSemaphoreLedControl;
#endif