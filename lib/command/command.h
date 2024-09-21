#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "project_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "reSensor.h"
#include "reTgSend.h"
#include "sensors.h"
#include "reRangeMonitor.h" //??
#include "strings.h"
#include "def_consts.h"
#include "esp_timer.h"
#include "rLog.h"
#include "rTypes.h"
#include "rStrings.h"
#include "reStates.h"
#include "reEvents.h"
#include "reSysInfo.h"
#include "reWiFi.h"
/*for pinger*/
#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/icmp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "rePinger.h"
#include "reEsp32.h"
#include "reParams.h"

#define CONFIG_COMMAND_NOTIFICATION_REBOOT "Перезагрузка по команде пользователя через 10 секунд"
enum cmd_perm_level_type_t
{
    CMD_PERMISSION_DENIED,
    CMD_PERMISSION_USER,
    CMD_PERMISSION_ADMIN,
    CMD_PERMISSION_SU,
};

bool cmdTaskCreate(QueueHandle_t *cmdQueue);
u_int8_t permission_check(tgUpdateMessage_t *msg);
char *cmdProcessor(tgUpdateMessage_t *msg, u_int8_t perm_level);
void printFileInfo(tgUpdateMessage_t *msg);
char *statesGetStatus();

#endif // __COMMAND_H__