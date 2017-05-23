/**
* Реализация HTTP сервера
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#ifndef WS_HTTP_h
#define WS_HTTP_h
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "FS.h"

extern ESP8266WebServer server;
extern ESP8266HTTPUpdateServer httpUpdater;
extern bool isAP;
extern bool isConnect;
extern char m_file[];
extern bool     m_write;
extern uint16_t m_count;
extern uint32_t m_tm;
extern uint32_t tm;
extern float u1,i1,p1,e1;

#define DebugSerial Serial1


bool ConnectWiFi(void);
void ListWiFi(String &out);
void HTTP_begin(void);
void HTTP_loop();
void WiFi_begin(void);
void Time2Str(char *s,time_t t);
bool SetParamHTTP();
int  HTTP_isAuth();
void HTTP_handleRoot(void);
void HTTP_handleConfig(void);
void HTTP_handleLogin(void);
void HTTP_handleReboot(void);
void HTTP_handleView(void);
void HTTP_handleDownload(void);
void HTTP_handleDefault(void);
void HTTP_handleLogo(void);
void HTTP_gotoLogin();
int  HTTP_checkAuth(char *user);
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len,bool is_pass=false);
void HTTP_printHeader(String &out,const char *title, uint16_t refresh=0);
void HTTP_printTail(String &out);
//void SetPwm();
void HTTP_device(void);
void HTTP_handleGraph(void);
void HTTP_handleData(void);



#endif
