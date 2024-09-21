/*
  Telegram bot with telemetry by 
  vasily9k@gmail.com, https://github.com/vasily9k

  The code is based on the https://github.com/kotyara12/telemeter_dzen project by
  (с) 2021-2022 Razzhivin Alexander
  kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#include "stdlib.h"
#include "esp_log.h"
#include "project_config.h"
#include "def_consts.h"
#include "rLog.h"
#include "rStrings.h"
#include "reLed.h"
#include "reNvs.h"
#include "reStates.h"
#include "reParams.h"
#include "reEvents.h"
#include "reEsp32.h"
#include "reWiFi.h"
#include "reSNTP.h"
#ifndef CONFIG_MQTT1_OFF
#include "reMqtt.h"
#endif 
#include "reSysInfo.h"
#include "reScheduler.h"
#include "reCerts.h"
#if CONFIG_PINGER_ENABLE
#include "rePinger.h"
#endif // CONFIG_PINGER_ENABLE
#if CONFIG_TELEGRAM_ENABLE
#include "reTgSend.h"
#endif // CONFIG_TELEGRAM_ENABLE
#if CONFIG_DATASEND_ENABLE
#include "reDataSend.h"
#endif // CONFIG_DATASEND_ENABLE
#if defined(CONFIG_GPIO_BUZZER) && (CONFIG_GPIO_BUZZER > -1)
#include "reBeep.h"
#endif // CONFIG_GPIO_BUZZER
#include "sensors.h"
#include "command.h"

QueueHandle_t _cmdQueue = nullptr;

// Main function
extern "C" { void app_main(void) 
{
  // Initializing logs and display firmware version
  rlog_empty();
  disbleEspIdfLogs();
  rloga_i("Firmware initialization, version %s", APP_VERSION);
  vTaskDelay(1);

  // Registering reboot handlers 
  // You can add up to 5 handlers in total, 1 is system (debugging), 4 remain for applications
  espRegisterSystemShutdownHandler(); // #1
  vTaskDelay(1);

  // Initializing the Global Certificate Store
  initTlsGlobalCAStore();
  vTaskDelay(1);

  // Initialization of the parameter storage system
  paramsInit();
  vTaskDelay(1);

  // Initializing LEDs
  ledSysInit(CONFIG_GPIO_SYSTEM_LED, true, CONFIG_LED_TASK_STACK_SIZE, nullptr);
  ledSysOn(false);
  vTaskDelay(1);

  // Starting the main event loop
  eventLoopCreate();
  vTaskDelay(1);

  // Initializing the device status monitoring system
  statesInit(true);
  vTaskDelay(1);

  // Registering the "minute timer" service and schedules. But not starting
  schedulerEventHandlerRegister();
  vTaskDelay(1);

  #if CONFIG_PINGER_ENABLE
    // Registration of the service of periodic checking of external servers and Internet access
    pingerEventHandlerRegister();
    vTaskDelay(1);
  #endif // CONFIG_PINGER_ENABLE

  // Registering and starting the time synchronization service
  sntpTaskCreate(true);
  vTaskDelay(1);

  #ifndef CONFIG_MQTT1_OFF
    // Registering and starting MQTT client
    mqttTaskStart(true);
    vTaskDelay(1);

    // Registering handlers for MQTT events (connection, disconnection, incoming messages)
    paramsEventHandlerRegister();
    vTaskDelay(1);
  #endif // CONFIG_MQTT1_OFF

  #if CONFIG_TELEGRAM_ENABLE
    // Registering and starting the notification service and processing received Telegram messages
    tgTaskCreate();
    vTaskDelay(1);
    tgTaskUpdatesCreate(&_cmdQueue);
    vTaskDelay(1);
  #endif // CONFIG_TELEGRAM_ENABLE

  #if CONFIG_DATASEND_ENABLE
    // Launching a service for sending data to external services
    dsTaskCreate(false);
    vTaskDelay(1);
  #endif // CONFIG_DATASEND_ENABLE

  #if defined(CONFIG_GPIO_BUZZER) && (CONFIG_GPIO_BUZZER > -1)
    // Starting the buzzer service
    beepTaskCreate(CONFIG_GPIO_BUZZER);
    vTaskDelay(1);
  #endif // CONFIG_GPIO_BUZZER

  // Starting the temperature control service
  sensorsTaskStart();
  vTaskDelay(1);

  // Launching the command processor
  cmdTaskCreate(&_cmdQueue);
  vTaskDelay(1);

  // Connecting to WiFi AP
  if (!wifiStart()) {
    ledSysBlinkOn(1, 100, 250);
  };
}}
