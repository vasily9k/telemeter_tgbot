/*
   -----------------------------------------------------------------------------------------------------------------------
   EN: Project configuration file, accessible from all libraries connected to the project
   RU: Файл конфигурации секретов проекта, он должен быть доступен из всех файлов проекта, в том числе и библиотек
   -----------------------------------------------------------------------------------------------------------------------
*/

#pragma once

#define CONFIG_SNTP_TIMEZONE "UTC+2"
#define CONFIG_SNTP_TIMEZONE_SECONDS 60*60*2

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------- EN - Wifi networks -----------------------------------------------------
// ------------------------------------------------ RU - WiFi сети -------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------
/** 
 * EN: Single network mode
 * --------------------
 * Uncomment CONFIG_WIFI_SSID and CONFIG_WIFI_PASS to disable automatic switching between wifi networks
 * 
 * RU: Режим для одной сети
 * --------------------
 * Раскомментируйте CONFIG_WIFI_SSID и CONFIG_WIFI_PASS, чтобы отключить автоматическое переключение между wifi-сетями
 * */
// #define CONFIG_WIFI_SSID "wifi_network_name"
// #define CONFIG_WIFI_PASS "wifi_password"

/** 
 * EN: Multi-network mode
 * --------------------
 * You can define from one to five networks. If it is not possible to connect to one of the networks, ESP will try to connect to the next one.
 * The number of a successful connection is remembered and used later (until the next failure).
 * This allows you to move the device from one building to another without reflashing and reconfiguring it. 
 * Just define all possible networks in advance.
 * 
 * RU: Режим для нескольких сетей
 * --------------------
 * Вы можете определенить от одной до пяти сетей. При невозможности подключиться к одной из сетей, ESP попытается поключиться к следующей. 
 * Номер успешного подключения запоминается и используется в дальнейшем (до следущего сбоя). 
 * Это позволяет переносить устройство из одного здания в другое, не перепрошивая и перенастраивая его. 
 * Просто заранее определите все возможные сети.
 * */
#define CONFIG_WIFI_1_SSID "WIFI1"
#define CONFIG_WIFI_1_PASS "11111111"
#define CONFIG_WIFI_2_SSID "WIFI2"
#define CONFIG_WIFI_2_PASS "22222222"
#define CONFIG_WIFI_3_SSID "WIFI3"
#define CONFIG_WIFI_3_PASS "33333333"
#define CONFIG_WIFI_4_SSID "WIFI4"
#define CONFIG_WIFI_4_PASS "44444444"
#define CONFIG_WIFI_5_SSID "WIFI5"
#define CONFIG_WIFI_5_PASS "55555555"


/********************* primary MQTT server ************************/
#define CONFIG_MQTT1_HOST "host.mqtt.net"
#define CONFIG_MQTT1_PORT_TCP 443
#define CONFIG_MQTT1_PORT_TLS 443
#define CONFIG_MQTT1_USERNAME "username"
#define CONFIG_MQTT1_PASSWORD "password"


// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------- EN - Telegram secrets ---------------------------------------------------
// ------------------------------------------- RU - Токен и chat_id-s в Telegram -----------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------
// EN: Telegram API bot token
// RU: Токен бота API Telegram
//
// @xxx_bot
#define CONFIG_TELEGRAM_TOKEN "12345678900:ABcdkwrofjwrifworighowgoiwrgowigjoiwr"

// Allowed Chat or group ID
#define CONFIG_TELEGRAM_CHAT_ID_MAIN     "1111111111" 
#define CONFIG_TELEGRAM_CHAT_ID_SERVICE  "1111111111"

#define CONFIG_TELEGRAM_CHAT_ID_PARAMS   CONFIG_TELEGRAM_CHAT_ID_SERVICE
#define CONFIG_TELEGRAM_CHAT_ID_SECURITY CONFIG_TELEGRAM_CHAT_ID_MAIN
// EN: Device name (published as message header)
// RU: Название устройства (публикуется в качестве заголовка сообщения)
#define CONFIG_TELEGRAM_DEVICE "🏡 Мой дом"
