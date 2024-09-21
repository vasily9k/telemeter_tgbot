#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "rLog.h"
#include "rStrings.h"
#include "reEsp32.h"
#ifndef CONFIG_MQTT1_OFF
#include "reMqtt.h"
#endif 
#include "reBeep.h"
#include "reLed.h"
#include "reGpio.h"
#include "reLoadCtrl.h"

static const char* logTAG = "SNS";

static const char* sensorsTaskName = "sensors";
static TaskHandle_t _sensorsTask = nullptr;
static EventGroupHandle_t _sensorsFlags = nullptr;
static bool _sensorsNeedStore = false;

// Служебные биты
#define FLG_STORE_DATA            BIT0
#define FLG_PARAMS_CHANGED        BIT1
#define FLG_TIME_CHANGED          BIT2

// События на входах
#define FLG_INPUT_DOORBTN         BIT3
#define FLG_INPUT_GAS             BIT4

#define FLGS_WAIT                 (FLG_TIME_CHANGED | FLG_INPUT_DOORBTN | FLG_INPUT_GAS)
#define FLGS_CLEAR                0x00FFFFFF

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------ Параметры ------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static paramsGroupHandle_t pgSensors;
static paramsGroupHandle_t pgIntervals;

void sensorsInitParameters()
{
  // Группы
  pgSensors = paramsRegisterGroup(nullptr, 
    CONFIG_SENSOR_PGROUP_ROOT_KEY, CONFIG_SENSOR_PGROUP_ROOT_TOPIC, CONFIG_SENSOR_PGROUP_ROOT_FRIENDLY);
  pgIntervals = paramsRegisterGroup(pgSensors, 
    CONFIG_SENSOR_PGROUP_INTERVALS_KEY, CONFIG_SENSOR_PGROUP_INTERVALS_TOPIC, CONFIG_SENSOR_PGROUP_INTERVALS_FRIENDLY);

  if (pgIntervals) {
    // Период чтения данных с сенсоров
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U32, nullptr, pgIntervals,
      CONFIG_SENSOR_PARAM_INTERVAL_READ_KEY, CONFIG_SENSOR_PARAM_INTERVAL_READ_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&iSensorsReadInterval);

    // Период публикации данных с сенсоров на MQTT
    paramsRegisterValue(OPT_KIND_PARAMETER, OPT_TYPE_U32, nullptr, pgIntervals,
      CONFIG_SENSOR_PARAM_INTERVAL_MQTT_KEY, CONFIG_SENSOR_PARAM_INTERVAL_MQTT_FRIENDLY,
      CONFIG_MQTT_PARAMS_QOS, (void*)&iMqttPubInterval);
  };
}

// -----------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------- Сенсоры ------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

bool sensorsPublish(rSensor *sensor, char* topic, char* payload, const bool free_topic, const bool free_payload)
{
  #if !defined(CONFIG_MQTT1_OFF)
    return mqttPublish(topic, payload, CONFIG_MQTT_SENSORS_QOS, CONFIG_MQTT_SENSORS_RETAINED, free_topic, free_payload);
  #else
    return true;
  #endif // CONFIG_MQTT1_OFF
}

void sensorsMqttTopicsCreate(bool primary)
{
  sensorDS18B20int.topicsCreate(primary);
  sensorDS18B20ext.topicsCreate(primary);
}

void sensorsMqttTopicsFree()
{
  sensorDS18B20int.topicsFree();
  sensorDS18B20ext.topicsFree();
}

void sensorsMqttPublish()
{
  sensorDS18B20int.publishData(false);
  sensorDS18B20ext.publishData(false);
}

void sensorsStoreData()
{
  sensorDS18B20int.nvsStoreExtremums(SENSOR_DS18B20INT_KEY);
  sensorDS18B20ext.nvsStoreExtremums(SENSOR_DS18B20EXT_KEY);
}


void sensorsInitSensors()
{
  sensorDS18B20int.setSensorItems(&siDS18B20intTemp);
  sensorDS18B20int.registerParameters(pgSensors, SENSOR_DS18B20INT_KEY, SENSOR_DS18B20INT_TOPIC, SENSOR_DS18B20INT_NAME);
  sensorDS18B20int.nvsRestoreExtremums(SENSOR_DS18B20INT_KEY);
  sensorDS18B20int.sensorStart();

  sensorDS18B20ext.setSensorItems(&siDS18B20extTemp);
  sensorDS18B20ext.registerParameters(pgSensors, SENSOR_DS18B20EXT_KEY, SENSOR_DS18B20EXT_TOPIC, SENSOR_DS18B20EXT_NAME);
  sensorDS18B20ext.nvsRestoreExtremums(SENSOR_DS18B20EXT_KEY);
  sensorDS18B20ext.sensorStart();

}

void sensorsSensorsResetExtremumsAll(uint8_t mode)
{
  switch (mode) {
    case 1:
      sensorDS18B20int.resetExtremumsDaily();
      sensorDS18B20ext.resetExtremumsDaily();
      break;
    case 2:
      sensorDS18B20int.resetExtremumsWeekly();
      sensorDS18B20ext.resetExtremumsWeekly();
      break;
    case 3:
      sensorDS18B20int.resetExtremumsEntirely();
      sensorDS18B20ext.resetExtremumsEntirely();
      break;
    default:
      sensorDS18B20int.resetExtremumsTotal();
      sensorDS18B20ext.resetExtremumsTotal();
      break;
  };
}

void sensorsResetExtremumsSensor(rSensor* sensor, const char* sensor_name, uint8_t mode) 
{ 
  if (mode == 0) {
    sensor->resetExtremumsTotal();
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_TOTAL_DEV, sensor_name);
    #endif // CONFIG_TELEGRAM_ENABLE
  } else if (mode == 1) {
    sensor->resetExtremumsDaily();
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_DAILY_DEV, sensor_name);
    #endif // CONFIG_TELEGRAM_ENABLE
  } else if (mode == 2) {
    sensor->resetExtremumsWeekly();
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_WEEKLY_DEV, sensor_name);
    #endif // CONFIG_TELEGRAM_ENABLE
  } else if (mode == 3) {
    sensor->resetExtremumsEntirely();
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_ENTIRELY_DEV, sensor_name);
    #endif // CONFIG_TELEGRAM_ENABLE
  };
}

void sensorsResetExtremumsSensors(uint8_t mode)
{
  if (mode == 0) {
    sensorsSensorsResetExtremumsAll(0);
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_TOTAL_ALL);
    #endif // CONFIG_TELEGRAM_ENABLE
  } else if (mode == 1) {
    sensorsSensorsResetExtremumsAll(1);
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_DAILY_ALL);
    #endif // CONFIG_TELEGRAM_ENABLE
  } else if (mode == 2) {
    sensorsSensorsResetExtremumsAll(2);
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_WEEKLY_ALL);
    #endif // CONFIG_TELEGRAM_ENABLE
  } else if (mode == 3) {
    sensorsSensorsResetExtremumsAll(3);
    #if CONFIG_TELEGRAM_ENABLE
      tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
        CONFIG_MESSAGE_TG_SENSOR_CLREXTR_ENTIRELY_ALL);
    #endif // CONFIG_TELEGRAM_ENABLE
  };
};

bool sensorsSensorsResetExtremumsSelected(char * topic, uint8_t imode)
{
  if (strcasecmp(topic, SENSOR_DS18B20INT_TOPIC) == 0) {
    sensorsResetExtremumsSensor(&sensorDS18B20int, SENSOR_DS18B20INT_TOPIC, imode);
    return true;
  } else if (strcasecmp(topic, SENSOR_DS18B20EXT_TOPIC) == 0) {
    sensorsResetExtremumsSensor(&sensorDS18B20ext, SENSOR_DS18B20EXT_TOPIC, imode);
    return true;
  };
  return false;
}

void sensorsReadSensors()
{
  rlog_i(logTAG, "Read data from sensors...");

  sensorDS18B20int.readData();
  if (sensorDS18B20int.getStatus() == SENSOR_STATUS_OK) {
    rlog_i("DS18B20 INT", "Values raw: %.1f °С | out: %.1f °С | min: %.1f °С | max: %.1f °С", 
      sensorDS18B20int.getTemperature(false).rawValue, 
      sensorDS18B20int.getTemperature(false).filteredValue,
      sensorDS18B20int.getItemExtremumsDaily(0, false).minValue.filteredValue,
      sensorDS18B20int.getItemExtremumsDaily(0, false).maxValue.filteredValue);
  };

  sensorDS18B20ext.readData();
  if (sensorDS18B20ext.getStatus() == SENSOR_STATUS_OK) {
    rlog_i("DS18B20 EXT", "Values raw: %.1f °С | out: %.1f °С | min: %.1f °С | max: %.1f °С", 
      sensorDS18B20ext.getTemperature(false).rawValue, 
      sensorDS18B20ext.getTemperature(false).filteredValue,
      sensorDS18B20ext.getItemExtremumsDaily(0, false).minValue.filteredValue,
      sensorDS18B20ext.getItemExtremumsDaily(0, false).maxValue.filteredValue);
  };

}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- Internal GPIO ----------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------



// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------ Термостат ------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

// Простейший термостат (на фиксированную температуру 25 градусов)
void tempControl()
{
  if (sensorDS18B20ext.getStatus() == SENSOR_STATUS_OK) {
    if (sensorDS18B20ext.getTemperature(false).filteredValue < 27.0) {
      rlog_i(logTAG, "Boiler enabled");
      //lcBoiler.loadSetState(true, false, true);
      // gpio_set_level((gpio_num_t)CONFIG_GPIO_BOILER, CONFIG_GPIO_LEVEL_OUTPUTS_ON);
    } else {
      rlog_i(logTAG, "Boiler disabled");
      //lcBoiler.loadSetState(false, false, true);
      // gpio_set_level((gpio_num_t)CONFIG_GPIO_BOILER, CONFIG_GPIO_LEVEL_OUTPUTS_OFF);
    };
  };
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- Event Handlers ---------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

void evhdlTimeEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if (event_id == RE_TIME_EVERY_MINUTE) {
    xEventGroupSetBits(_sensorsFlags, FLG_TIME_CHANGED);
  };
  if (event_id == RE_TIME_START_OF_DAY) {
    xEventGroupSetBits(_sensorsFlags, FLG_STORE_DATA);
  };
  //lcBoiler.countersTimeEventHandler(event_id, event_data);
}

void evhdlMqttEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if (event_id == RE_MQTT_CONNECTED) {
    re_mqtt_event_data_t* data = (re_mqtt_event_data_t*)event_data;
    sensorsMqttTopicsCreate(data->primary);
  } 
  else if ((event_id == RE_MQTT_CONN_LOST) || (event_id == RE_MQTT_CONN_FAILED)) {
    sensorsMqttTopicsFree();
  }
}

void evhdlParamsEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if (event_id == RE_PARAMS_CHANGED) {
    xEventGroupSetBits(_sensorsFlags, FLG_PARAMS_CHANGED);
  };
}

void evhdlCommandsEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if ((event_id == RE_SYS_COMMAND) && (event_data)) {
    char* buf = malloc_string((char*)event_data);
    if (buf != nullptr) {
      const char* seps = " ";
      char* cmd = nullptr;
      char* mode = nullptr;
      char* sensor = nullptr;
      uint8_t imode = 0;
      cmd = strtok(buf, seps);
      
      // Команды сброса датчиков
      if ((cmd != nullptr) && (strcasecmp(cmd, CONFIG_SENSOR_COMMAND_EXTR_RESET) == 0)) {
        rlog_i(logTAG, "Reset extremums: %s", buf);
        sensor = strtok(nullptr, seps);
        if (sensor != nullptr) {
          mode = strtok(nullptr, seps);
        };
      
        // Опрделение режима сброса
        if (mode == nullptr) {
          // Возможно, вторым токеном идет режим, в этом случае сбрасываем для всех сенсоров
          if (sensor) {
            if (strcasecmp(sensor, CONFIG_SENSOR_EXTREMUMS_DAILY) == 0) {
              sensor = nullptr;
              imode = 1;
            } else if (strcasecmp(sensor, CONFIG_SENSOR_EXTREMUMS_WEEKLY) == 0) {
              sensor = nullptr;
              imode = 2;
            } else if (strcasecmp(sensor, CONFIG_SENSOR_EXTREMUMS_ENTIRELY) == 0) {
              sensor = nullptr;
              imode = 3;
            };
          };
        } else if (strcasecmp(mode, CONFIG_SENSOR_EXTREMUMS_DAILY) == 0) {
          imode = 1;
        } else if (strcasecmp(mode, CONFIG_SENSOR_EXTREMUMS_WEEKLY) == 0) {
          imode = 2;
        } else if (strcasecmp(mode, CONFIG_SENSOR_EXTREMUMS_ENTIRELY) == 0) {
          imode = 3;
        };

        // Определение сенсора
        if ((sensor == nullptr) || (strcasecmp(sensor, CONFIG_SENSOR_COMMAND_SENSORS_PREFIX) == 0)) {
          sensorsResetExtremumsSensors(imode);
        } else {
          if (!sensorsSensorsResetExtremumsSelected(sensor, imode)) {
            rlog_w(logTAG, "Sensor [ %s ] not found", sensor);
            #if CONFIG_TELEGRAM_ENABLE
              tgSend(CONFIG_SENSOR_COMMAND_KIND, CONFIG_SENSOR_COMMAND_PRIORITY, CONFIG_SENSOR_COMMAND_NOTIFY, CONFIG_TELEGRAM_DEVICE,
                CONFIG_MESSAGE_TG_SENSOR_CLREXTR_UNKNOWN, sensor);
            #endif // CONFIG_TELEGRAM_ENABLE
          };
        };
      };
    };
    if (buf != nullptr) free(buf);
  };
}

void evhdlOtaEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if ((event_id == RE_SYS_OTA) && (event_data)) {
    re_system_event_data_t* data = (re_system_event_data_t*)event_data;
    if (data->type == RE_SYS_SET) {
      //lcBoiler.loadSetState(false, true, true);
      sensorsTaskSuspend();
    } else {
      sensorsTaskResume();
    };
  };
}

void evhdlStoreData()
{
  sensorsStoreData();
  //gpioStoreData();  
}

bool evhdlEventHandlersRegister()
{
  return eventHandlerRegister(RE_MQTT_EVENTS, ESP_EVENT_ANY_ID, &evhdlMqttEventHandler, nullptr) 
      && eventHandlerRegister(RE_TIME_EVENTS, ESP_EVENT_ANY_ID, &evhdlTimeEventHandler, nullptr)
      && eventHandlerRegister(RE_PARAMS_EVENTS, ESP_EVENT_ANY_ID, &evhdlParamsEventHandler, nullptr)
      && eventHandlerRegister(RE_SYSTEM_EVENTS, RE_SYS_COMMAND, &evhdlCommandsEventHandler, nullptr)
      && eventHandlerRegister(RE_SYSTEM_EVENTS, RE_SYS_OTA, &evhdlOtaEventHandler, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------- Задача --------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

void sensorsTaskExec(void *pvParameters)
{
  // -------------------------------------------------------------------------------------------------------
  // Инициализация устройств и сенсоров
  // -------------------------------------------------------------------------------------------------------
  //gpioInit();
  //sensorsInitModbus();
  sensorsInitSensors();
  espRegisterShutdownHandler(evhdlStoreData); // #2

  // -------------------------------------------------------------------------------------------------------
  // Таймеры публикции данных с сенсоров
  // -------------------------------------------------------------------------------------------------------
  static esp_timer_t mqttPubTimer;
  timerSet(&mqttPubTimer, iMqttPubInterval*1000);
  
  static TickType_t waitTicks = 0;
  static TickType_t readTicks = 0;
  static TickType_t currTicks = 0;
  static EventBits_t bits = 0;
  static uint32_t beepFreq = 0;
  static bool out_state = false;
  while (1) {
    // Ждем тайамута или любого сигнального бита (что наступит первым)
    bits = xEventGroupWaitBits(_sensorsFlags, FLGS_WAIT, pdFALSE, pdFALSE, waitTicks);
    // Фиксируем время начала данного рабочего цикла
    readTicks = xTaskGetTickCount(); 
    // Очищаем сигнальные биты (их состояние мы сохранили в bits)
    xEventGroupClearBits(_sensorsFlags, FLGS_CLEAR);

    // Обработка событий по встроенным входам
    //if ((bits & FLG_INPUT_DOORBTN) > 0) gpioDoorButtonPress();
    //if ((bits & FLG_INPUT_GAS) > 0) gpioGasLeakSignal();

    // Раз в сутки принудительно сохраням экстремумы с сенсоров в NVS
    if ((bits & FLG_STORE_DATA) > 0) sensorsStoreData();

    // Чтение данных с сенсоров
    sensorsReadSensors();

    // Простейший термостат
    tempControl();


    // Считаем время ожидания следующего цикла
    TickType_t currTicks = xTaskGetTickCount();    
    if ((currTicks - readTicks) >= pdMS_TO_TICKS(iSensorsReadInterval*1000)) {
      waitTicks = 0;
    } else {
      waitTicks = pdMS_TO_TICKS(iSensorsReadInterval*1000) - (currTicks - readTicks);
    };
  };

  vTaskDelete(nullptr);
  espRestart(RR_ERROR);
}

bool sensorsTaskStart()
{
  #if CONFIG_SENSORS_STATIC_ALLOCATION
    static StaticEventGroup_t sensorsFlagsBuffer;
    static StaticTask_t sensorsTaskBuffer;
    static StackType_t sensorsTaskStack[CONFIG_SENSORS_TASK_STACK_SIZE];
    _sensorsFlags = xEventGroupCreateStatic(&sensorsFlagsBuffer);
    _sensorsTask = xTaskCreateStaticPinnedToCore(sensorsTaskExec, sensorsTaskName, 
      CONFIG_SENSORS_TASK_STACK_SIZE, NULL, CONFIG_SENSORS_TASK_PRIORITY, sensorsTaskStack, &sensorsTaskBuffer, CONFIG_SENSORS_TASK_CORE);
  #else
    _sensorsFlags = xEventGroupCreate();
    xTaskCreatePinnedToCore(sensorsTaskExec, sensorsTaskName, 
      CONFIG_SENSORS_TASK_STACK_SIZE, NULL, CONFIG_SENSORS_TASK_PRIORITY, &_sensorsTask, CONFIG_SENSORS_TASK_CORE);
  #endif // CONFIG_SENSORS_STATIC_ALLOCATION
  if (_sensorsFlags && _sensorsTask) {
    rloga_i("Task [ %s ] has been successfully created and started", sensorsTaskName);
    return evhdlEventHandlersRegister();
  }
  else {
    rloga_e("Failed to create a task for processing sensor readings!");
    return false;
  };
}

bool sensorsTaskSuspend()
{
  if ((_sensorsTask) && (eTaskGetState(_sensorsTask) != eSuspended)) {
    vTaskSuspend(_sensorsTask);
    if (eTaskGetState(_sensorsTask) == eSuspended) {
      rloga_d("Task [ %s ] has been suspended", sensorsTaskName);
      return true;
    } else {
      rloga_e("Failed to suspend task [ %s ]!", sensorsTaskName);
    };
  };
  return false;
}

bool sensorsTaskResume()
{
  if ((_sensorsTask) && (eTaskGetState(_sensorsTask) == eSuspended)) {
    vTaskResume(_sensorsTask);
    if (eTaskGetState(_sensorsTask) != eSuspended) {
      rloga_i("Task [ %s ] has been successfully resumed", sensorsTaskName);
      return true;
    } else {
      rloga_e("Failed to resume task [ %s ]!", sensorsTaskName);
    };
  };
  return false;
}