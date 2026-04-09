#include "neo_blinky.h"


void neo_blinky(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    // Set all pixels to off to start
    strip.clear();
    strip.show();

    while(1) {                          
        strip.setPixelColor(0, strip.Color(255, 0, 0)); // Set pixel 0 to red
        strip.show(); // Update the strip

        // Wait for 500 milliseconds
        vTaskDelay(500);

        // Set the pixel to off
        strip.setPixelColor(0, strip.Color(0, 0, 0)); // Turn pixel 0 off
        strip.show(); // Update the strip

        // Wait for another 500 milliseconds
        vTaskDelay(500);
    }
}




void vTaskNeoBlinky(void *pvParameters) {

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.setBrightness(50); // Đặt độ sáng vừa phải để bảo vệ mắt
    strip.show();
    
    SensorData_t receivedData;
    NeoBlinkyState_t currentState = LED_STATE_NORMAL_HUMI;
    
    TickType_t blinkInterval = pdMS_TO_TICKS(1000); 
    TickType_t lastToggleTime = xTaskGetTickCount();
    bool ledState = false;
    uint32_t currentColor = strip.Color(0, 255, 0); // Mặc định Lục

    while (1) {
        if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
            
            if (receivedData.humidity >= HUMIDITY_VERY_HUMID) {
                currentState = LED_STATE_HUMIDITY_VERY_HUMID;
            } 
            else if (receivedData.humidity >= HUMIDITY_HUMID) {
                currentState = LED_STATE_HUMIDITY_HUMID;
            } 
            else if (receivedData.humidity <= HUMIDITY_VERY_DRY) {
                currentState = LED_STATE_HUMIDITY_VERY_DRY;
            } 
            else if (receivedData.humidity <= HUMIDITY_DRY) {
                currentState = LED_STATE_HUMIDITY_DRY;
            } 
            else {
                currentState = LED_STATE_NORMAL_HUMI; 
            }
        }





        switch (currentState) {
            case LED_STATE_HUMIDITY_VERY_HUMID:  
                currentColor = strip.Color(128, 0, 128); 
                blinkInterval = pdMS_TO_TICKS(200);      
                break;
            case LED_STATE_HUMIDITY_HUMID: 
                currentColor = strip.Color(0, 0, 255);   
                blinkInterval = pdMS_TO_TICKS(600);      
                break;
            case LED_STATE_NORMAL_HUMI:    
                currentColor = strip.Color(0, 255, 0);   
                blinkInterval = pdMS_TO_TICKS(1500);     
                break;
            case LED_STATE_HUMIDITY_DRY: 
                currentColor = strip.Color(255, 255, 0); 
                blinkInterval = pdMS_TO_TICKS(600);      
                break;
            case LED_STATE_HUMIDITY_VERY_DRY: 
                currentColor = strip.Color(255, 0, 0);   
                blinkInterval = pdMS_TO_TICKS(200);      
                break;
        }







        if ((xTaskGetTickCount() - lastToggleTime) >= blinkInterval) {
            ledState = !ledState;
            if (ledState) {
                strip.setPixelColor(0, currentColor);
            } else {
                strip.setPixelColor(0, strip.Color(0, 0, 0)); 
            }
            strip.show();
            lastToggleTime = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}