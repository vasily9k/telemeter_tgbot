#ifndef __SENSORS_H__
#define __SENSORS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "project_config.h"
#include "def_consts.h"
#include "esp_timer.h"
#include "rTypes.h" 
#include "reSensor.h" 
#include "reDS18x20.h"

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------- Сенсоры -------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

bool sensorsPublish(rSensor *sensor, char* topic, char* payload, const bool free_topic, const bool free_payload);

#define SENSORS_DEFAULT_READ_INTERVAL       10    // s
#define SENSORS_MINIMAL_READ_INTERVAL       1000  // ms
#define SENSORS_TOPIC_LOCAL                 0
#define SENSORS_FILTER_MODE                 SENSOR_FILTER_RAW
#define SENSORS_FILTER_SIZE                 0
#define SENSORS_ERRORS_LIMIT                16

// Период опроса датчиков (чтения данных с сенсоров) в секундах
static uint32_t iSensorsReadInterval = SENSORS_DEFAULT_READ_INTERVAL;
// Период публикации данных с сенсоров на MQTT
static uint32_t iMqttPubInterval = CONFIG_MQTT_SENSORS_SEND_INTERVAL;

// DS18B20, расположенный на плате (проверка шины 1-Wire #1)
#define SENSOR_DS18B20INT_IDS               1
#define SENSOR_DS18B20INT_NAME              "DS18B20 на плате"
#define SENSOR_DS18B20INT_KEY               "int"
#define SENSOR_DS18B20INT_TOPIC             "ds18b20_int"
#define SENSOR_DS18B20INT_INDEX             1
#define SENSOR_DS18B20INT_ADDR              ONEWIRE_NONE
#define SENSOR_DS18B20INT_PIN               (gpio_num_t)CONFIG_GPIO_1WIRE_DS1820
#define SENSOR_DS18B20INT_RESOLUTION        DS18x20_RESOLUTION_12_BIT
#define SENSOR_DS18B20INT_READ_SCRATCHPAD   true

static DS18x20 sensorDS18B20int(SENSOR_DS18B20INT_IDS, 
  SENSOR_DS18B20INT_PIN, SENSOR_DS18B20INT_ADDR, SENSOR_DS18B20INT_INDEX, SENSOR_DS18B20INT_RESOLUTION, SENSOR_DS18B20INT_READ_SCRATCHPAD,
  SENSOR_DS18B20INT_NAME, SENSOR_DS18B20INT_TOPIC, SENSORS_TOPIC_LOCAL, SENSORS_MINIMAL_READ_INTERVAL, SENSORS_ERRORS_LIMIT,
  nullptr, sensorsPublish);
static rTemperatureItem siDS18B20intTemp(&sensorDS18B20int, 
  CONFIG_SENSOR_TEMP_KEY, CONFIG_SENSOR_TEMP_NAME, CONFIG_SENSOR_TEMP_FRIENDLY, CONFIG_FORMAT_TEMP_UNIT,
  SENSORS_FILTER_MODE, SENSORS_FILTER_SIZE, 
  CONFIG_FORMAT_TEMP_VALUE, CONFIG_FORMAT_TEMP_STRING);

// DS18B20 подключенный к разъему TM  (проверка шины 1-Wire #2)
#define SENSOR_DS18B20EXT_IDS               2
#define SENSOR_DS18B20EXT_NAME              "DS18B20 на разъеме ТМ"
#define SENSOR_DS18B20EXT_KEY               "ext"
#define SENSOR_DS18B20EXT_TOPIC             "ds18b20_ext"
#define SENSOR_DS18B20EXT_INDEX             1
#define SENSOR_DS18B20EXT_ADDR              ONEWIRE_NONE
#define SENSOR_DS18B20EXT_PIN               (gpio_num_t)CONFIG_GPIO_1WIRE_TM
#define SENSOR_DS18B20EXT_RESOLUTION        DS18x20_RESOLUTION_12_BIT
#define SENSOR_DS18B20EXT_READ_SCRATCHPAD   true

static DS18x20 sensorDS18B20ext(SENSOR_DS18B20EXT_IDS, 
  SENSOR_DS18B20EXT_PIN, SENSOR_DS18B20EXT_ADDR, SENSOR_DS18B20EXT_INDEX, SENSOR_DS18B20EXT_RESOLUTION, SENSOR_DS18B20EXT_READ_SCRATCHPAD,
  SENSOR_DS18B20EXT_NAME, SENSOR_DS18B20EXT_TOPIC, SENSORS_TOPIC_LOCAL, SENSORS_MINIMAL_READ_INTERVAL, SENSORS_ERRORS_LIMIT,
  nullptr, sensorsPublish);
static rTemperatureItem siDS18B20extTemp(&sensorDS18B20ext, 
  CONFIG_SENSOR_TEMP_KEY, CONFIG_SENSOR_TEMP_NAME, CONFIG_SENSOR_TEMP_FRIENDLY, CONFIG_FORMAT_TEMP_UNIT,
  SENSORS_FILTER_MODE, SENSORS_FILTER_SIZE, 
  CONFIG_FORMAT_TEMP_VALUE, CONFIG_FORMAT_TEMP_STRING);

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------- Задача --------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

bool sensorsTaskStart();
bool sensorsTaskSuspend();
bool sensorsTaskResume();

#endif // __SENSORS_H__