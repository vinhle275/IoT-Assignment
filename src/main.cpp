#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "mainserver.h"
#include "tinyml.h"
#include "coreiot.h"

// include task
// #include "task_check_info.h"
// #include "task_toogle_boot.h"
// #include "task_wifi.h"
// #include "task_webserver.h"
// #include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);
  // check_info_File(0);
  
  sensorQueue = xQueueCreate(1, sizeof(SensorData_t));
  weatherQueue = xQueueCreate(1, sizeof(WeatherData_t));
  wifiConfigQueue = xQueueCreate(1, sizeof(WifiConfig_t));
  coreIotQueue = xQueueCreate(1, sizeof(CoreIotConfig_t));
  localMqttQueue = xQueueCreate(1, sizeof(MqttLocalConfig_t));


  CoreIotConfig_t defaultCore = {"app.coreiot.io", "2a9lqf9pvmor6aui0bts"};
  xQueueOverwrite(coreIotQueue, &defaultCore);

  MqttLocalConfig_t defaultLocal = {"ESP32_001", "1234567890", "192.168.1.3", 1883};
  xQueueOverwrite(localMqttQueue, &defaultLocal);

  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 2048, NULL, 2, NULL);
  xTaskCreate(main_server_task, "Task Main Server" ,10240  ,NULL  ,2 , NULL);
  xTaskCreate( tiny_ml_task, "Tiny ML Task" ,4096  ,NULL  ,2 , NULL);
  xTaskCreate(coreiot_task, "CoreIOT Task" ,4096  ,NULL  ,2 , NULL);
  // xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}

void loop()
{
  // if (check_info_File(1))
  // {
  //   if (!Wifi_reconnect())
  //   {
  //     Webserver_stop();
  //   }
  //   else
  //   {
  //     //CORE_IOT_reconnect();
  //   }
  // }
  // Webserver_reconnect();
}