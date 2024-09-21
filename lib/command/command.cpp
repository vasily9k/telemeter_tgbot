#include "command.h"

#define COMMAND_OTA_URL_TMPL "https://api.telegram.org/file/bot" CONFIG_TELEGRAM_TOKEN "/documents/file_%d.bin"

static const char *logTAG = "CMD";
static const char *cmdTaskName = "cmd_processor";
TaskHandle_t _cmdTask;
QueueHandle_t *_cmdQueueP = nullptr;

void restart_task(void *pvParameters)
{
  rloga_e(CONFIG_COMMAND_NOTIFICATION_REBOOT);
  vTaskDelay(pdMS_TO_TICKS(10000));
  espRestart(RR_COMMAND_RESET);
  vTaskDelete(nullptr);
}

void cmdTaskExec(void *pvParameters)
{
  tgUpdateMessage_t *cmdMsg;
  while (true)
  {
    if (xQueueReceive(*_cmdQueueP, &cmdMsg, portMAX_DELAY) == pdPASS)
    {
      tgUpdateMessage_t cmdMsgReceived;
      memset(&cmdMsgReceived, 0, sizeof(tgUpdateMessage_t));

      cmdMsgReceived.text = nullptr;
      cmdMsgReceived.file = nullptr;
      cmdMsgReceived.chat_id = cmdMsg->chat_id;
      cmdMsgReceived.from_id = cmdMsg->from_id;
      cmdMsgReceived.date = cmdMsg->date;
      cmdMsgReceived.update_id = cmdMsg->update_id;
      cmdMsgReceived.type = cmdMsg->type;
      u_int8_t perm_level = permission_check(&cmdMsgReceived);
      char *answer = nullptr;

      if (perm_level > CMD_PERMISSION_DENIED)
      {
        switch (cmdMsgReceived.type)
        {
        case TG_MESSAGE_TEXT:
          cmdMsgReceived.text = cmdMsg->text;
          rlog_d(logTAG, "New command received: %s", cmdMsg->text);
          answer = cmdProcessor(&cmdMsgReceived, perm_level);
          break;
        case TG_MESSAGE_DOCUMENT:
          cmdMsgReceived.file = cmdMsg->file;
          printFileInfo(&cmdMsgReceived);
          tgGetFileApi(cmdMsg->file->id);
          break;
        case TG_MESSAGE_LINKTOFILE:
          cmdMsgReceived.text = cmdMsg->text;
          rlog_d(logTAG, "Upgrade link: %s", cmdMsg->text);
          answer = cmdProcessor(&cmdMsgReceived, perm_level);
          break;
        default:
          rlog_d(logTAG, "Не пойми что пришло");
          break;
        };
      }
      if (answer)
      {
        rlog_d(logTAG, "answer is: %s", answer);
        tgSend(MK_MAIN, MP_CRITICAL, true, CONFIG_TELEGRAM_DEVICE, "%s", answer);
        // free(answer);
      }
      else
      {
        rlog_d(logTAG, "answer is empty:");
      }

      // if (cmdMsg->text) free(cmdMsg->text);
      if (cmdMsg->file)
        free(cmdMsg->file);
      free(cmdMsg);
      cmdMsg = nullptr;
      // if (answer) free(answer);
    };
  };
}

bool cmdTaskCreate(QueueHandle_t *cmdQueue)
{
  _cmdQueueP = cmdQueue;
  if (!_cmdTask)
  {
    if (!*_cmdQueueP)
    {
      rloga_e("The _cmdQueueP is not available!");
      // eventLoopPostError(RE_SYS_TELEGRAM_ERROR, ESP_FAIL);
      return false;
    };

    xTaskCreatePinnedToCore(cmdTaskExec, cmdTaskName, 4096, nullptr, CONFIG_TASK_PRIORITY_TELEGRAM, &_cmdTask, CONFIG_TASK_CORE_TELEGRAM);
    if (!_cmdTask)
    {
      rloga_e("Failed to create command processor task!");
      // eventLoopPostError(RE_SYS_TELEGRAM_ERROR, ESP_FAIL);
      return false;
    }
    else
    {
      rloga_i("Task [ %s ] has been successfully started", cmdTaskName);
      // eventLoopPostError(RE_SYS_TELEGRAM_ERROR, ESP_OK);
      return true;
    };
  }
  else
  {
    return true;
  };
}

u_int8_t permission_check(tgUpdateMessage_t *msg)
{
  rlog_d(logTAG, "PERM Received chat_id: %" PRId64 ", from_id: %" PRId64 ", date: %d",
         msg->chat_id, msg->from_id, msg->date);
  return CMD_PERMISSION_SU;
}

char *cmdProcessor(tgUpdateMessage_t *msg, u_int8_t perm_level)
{
  if (strcmp(msg->text, "/status") == 0)
  {
    char *debug_heap = statesGetStatus();
    return debug_heap;
  }
  else if (strcmp(msg->text, "/hi") == 0)
  {
    return (char *)"Привет";
  }
  else if (strcmp(msg->text, "/reboot") == 0)
  {
    xTaskCreate(&restart_task, "restart_task", 2048, NULL, 5, NULL);
    return (char *)"Перезагрузка по команде пользователя";
  }
  else if (strcmp(msg->text, "/new_firmware_approve") == 0)
  {
    statesFirmwareVerifyCompete();
    return (char *)"Подтверждена работоспособность новой прошивки";
  }
  else if (strcmp(msg->text, "/taskinfo") == 0)
  {
    return sysinfoGetTaskList();
  }
  else if (strncmp(msg->text, "ping ", 5) == 0)
  {
    size_t host_name_length = strlen(msg->text) - 5;

    rlog_d(logTAG, "The length of the string is %zu bytes.\n", host_name_length);

    char *hostname = (char *)esp_calloc(1, host_name_length + 1);

    int ret = sscanf(msg->text, "ping %s", hostname);
    if (ret == 0) {
      return (char *)"Не могу выделить hostname";
    }
    rlog_d(logTAG, "hostname %s", hostname);

    pinger_data_t pdHostX;
    if (pingerInitSession(&pdHostX, hostname, 7010, 1) == ESP_OK)
    {
      pingerCheckHostEx(&pdHostX);
      return malloc_stringf("ping ok, hostname: %s", hostname);
    };
    return malloc_stringf("hostname: %s", hostname);
  }
  else if (strncmp(msg->text, "/upgrade_", 9) == 0)
  {
    int file_number = 0;
    int ret = sscanf(msg->text, "/upgrade_%d", &file_number);
    if (file_number == 0) // change to NAN!
      return (char *)"Ошибка со ссылкой на файл";

    //const char fmt[] = "Ссылка на файл:\ndocuments/file_%d.bin\nOTA started!";
    //int sz = snprintf(NULL, 0, fmt, file_number);
    //char *buf = (char *)esp_calloc(1, sz + 1);
    //snprintf(buf, sz + 1, fmt, file_number);

    char *ota_source = malloc_stringf(COMMAND_OTA_URL_TMPL, file_number);
    if (ota_source == nullptr)
    {
      rlog_e(logTAG, "Failed to allocate memory to link to ota file");
      return (char *)"Out of memory";
    };

    otaStart(ota_source);
    return (char *)"OTA started!";
  }
  else if (msg->chat_id == 0)
  {
     return msg->text;
  }
  else
  {
    static const char fmt[] = "Пришла неизвестная команда: %s";
    int sz = snprintf(NULL, 0, fmt, (const char *)msg->text);
    char *buf = (char *)esp_calloc(1, sz + 1);
    snprintf(buf, sz + 1, fmt, (const char *)msg->text);
    rlog_d(logTAG, "%s", buf);
    return buf;
  }
}

void printFileInfo(tgUpdateMessage_t *msg)
{
  if (msg->file)
  {
    rlog_d(logTAG, "printFileInfo name: %s,caption: %s, size: %d, id: %s",
           (msg->file->name) ? msg->file->name : "NO name",
           (msg->file->caption) ? msg->file->caption : "NO CAP",
           (msg->file->size) ? msg->file->size : 0,
           (msg->file->id) ? msg->file->id : "no id");
  }
  else
  {
    rlog_d(logTAG, "no msg->file");
  }
}

char *statesGetStatus()
{
  double heapTotal = (double)heap_caps_get_total_size(MALLOC_CAP_DEFAULT) / 1024;
  double heapFree = (double)heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024;
  double heapFreeMin = (double)heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT) / 1024;
  wifi_ap_record_t wf_info = wifiInfo();

  return malloc_stringf("FW: " APP_VERSION "\n"
                        "Heap. Total: %.1fkB. Free: %.2fkB (%.1f%%). FreeMin: %.2fkB (%.1f%%)\n"
                        "WI-FI. ssid: %s, rssi: %d, local IP: %s",
                        heapTotal,
                        heapFree, 100.0 * (heapFree / heapTotal),
                        heapFreeMin, 100.0 * (heapFreeMin / heapTotal),
                        wf_info.ssid, wf_info.rssi,
                        wifiGetLocalIP());
}
