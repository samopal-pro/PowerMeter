#ifndef WC_NTP_h
#define WC_NTP_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define NTP_TIMEOUT 600000 //10 минут
#define TZ          5      //Таймзона для Перми

extern time_t ntp_tm;
extern time_t ntp_last;
extern time_t uptime_tm;
extern uint8_t ntp_serial;
extern char   ntp_host[];

time_t GetNTP(void);
unsigned long sendNTPpacket(IPAddress& address);
void NTP_begin(void);
#endif
