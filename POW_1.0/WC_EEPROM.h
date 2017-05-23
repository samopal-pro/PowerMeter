/**
* Сохранение настроек в EEPROM
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#define DebugSerial Serial1
#ifndef WC_EEPROM_h
#define WC_EEPROM_h
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "WC_HTTP.h"
#include <EEPROM.h>


struct WC_NET_CONFIG{
// Наименование в режиме точки доступа  
   char ESP_NAME[32];
   char ESP_PASS[33];
// Параметры подключения в режиме клиента
   char AP_SSID[32];
   char AP_PASS[65];
   IPAddress IP;
   IPAddress MASK;
   IPAddress GW;
   char WEB_PASS[32];
   uint16_t SRC;   
};

extern struct WC_NET_CONFIG NetConfig;

void     EC_begin(void);
void     EC_read(void);
void     EC_save(void);
uint16_t EC_SRC(void);
void     EC_default(void);




#endif
