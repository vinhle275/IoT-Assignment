#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(0x27,16,2);


void temp_humi_monitor(void *pvParameters){

    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();

    lcd.begin();
    lcd.backlight(); 


    SensorData_t currentData;

    while (1){
        /* code */
        
        dht20.read();
        // Reading temperature in Celsius
        float temperature = dht20.getTemperature();
        // Reading humidity
        float humidity = dht20.getHumidity();

        

        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }else{
            currentData.temperature = temperature;
            currentData.humidity = humidity;
        }


        if (sensorQueue != NULL) {
            xQueueOverwrite(sensorQueue, &currentData);
        }

        
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");





        lcd.setCursor(0, 0);
        lcd.print("Temp:");
        lcd.print(temperature);
        lcd.setCursor(0, 1);
        lcd.print("Humi:");
        lcd.print(humidity);
        lcd.backlight(); 

        
        vTaskDelay(5000);
    }
    
}