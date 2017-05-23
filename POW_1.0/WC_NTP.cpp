#include "WC_NTP.h"

WiFiUDP udp;
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[ NTP_PACKET_SIZE]; 
uint8_t ntp_serial      = 1;
time_t ntp_tm           = 0;
time_t ntp_last         = 0;
time_t uptime_tm        = 0;
char   ntp_host[32];
unsigned int  localPort = 2390;      // local port to listen for UDP packets
const char NTP_SERVER1[] = "0.ru.pool.ntp.org";          
const char NTP_SERVER2[] = "1.ru.pool.ntp.org";  
const char NTP_SERVER3[] = "2.ru.pool.ntp.org";  

void NTP_begin(void){
   udp.begin(localPort);
}

/**
 * Посылаем и парсим запрос к NTP серверу
 */
time_t GetNTP(void) {
  IPAddress ntpIP;
  time_t tm = 0;
// Ротация NTP сервров из пула
  switch(ntp_serial){
     case 0:
        strcpy(ntp_host,NTP_SERVER1);
        ntp_serial = 1;
        break;
     case 1:
        strcpy(ntp_host,NTP_SERVER2);
        ntp_serial = 2;
        break;
     default:        
        strcpy(ntp_host,NTP_SERVER3);
        ntp_serial = 0;
        break;
       
  }
  WiFi.hostByName(ntp_host, ntpIP); 
  sendNTPpacket(ntpIP); 
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    return tm;
  }
  else {
// Читаем пакет в буфер    
    udp.read(packetBuffer, NTP_PACKET_SIZE); 
// 4 байта начиная с 40-го сождержат таймстамп времени - число секунд 
// от 01.01.1900   
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
// Конвертируем два слова в переменную long
    unsigned long secsSince1900 = highWord << 16 | lowWord;
// Конвертируем в UNIX-таймстамп (число секунд от 01.01.1970
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
// Делаем поправку на местную тайм-зону
    tm = epoch + TZ*3600;    
    ntp_last = tm;
  }
  return tm;
}

/**
 * Посылаем запрос NTP серверу на заданный адрес
 */
unsigned long sendNTPpacket(IPAddress& address)
{
// Очистка буфера в 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
// Формируем строку зыпроса NTP сервера
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
// Посылаем запрос на NTP сервер (123 порт)
  udp.beginPacket(address, 123); 
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
