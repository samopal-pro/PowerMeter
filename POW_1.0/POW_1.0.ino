/**
* Измеритель электроэнергии PowerMeter_1.0
* 
* Контроллер ESP8266F (4Мбайт)
* Модуль измерения электроэнергии PZEM004T
* Графический экран 240x320 ILI0341
* 
При недоступности WiFi подключения старт в режиме точки дступа
Авторизованный доступ к странице настроек. Пароль к WEB по умолчанию "admin". Настройки сохраняются в EEPROM
Обновление прошивки через WEB

* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include <arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FS.h>
#include "WC_EEPROM.h"
#include "WC_HTTP.h"
#include "WC_NTP.h"
#include "sav_button.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Fonts/FreeMono9pt7b.h";
#include "Fonts/FreeMonoBold12pt7b.h"
#include "Fonts/FreeMono18pt7b.h"


#include <SPI.h> 
#include "PZEM004T.h"

#define PIN_DC      5
#define PIN_RESET   4
#define PIN_CS      15

#define PAGE_SIZE   300

// Перегружаться, если в точке доступа
#define AP_REBOOT_TM 600

#define PIN_BUTTON 0

PZEM004T pzem(&Serial);
IPAddress ip(192,168,1,1);
uint32_t ms, ms1 = 0,ms2 = 0, ms3 = 0, ms4 = 0, ms_ok = 0;
uint32_t tm         = 0;
uint32_t t_cur      = 0;    
long  t_correct     = 0;


char     m_file[16];
uint16_t m_count = 0;
uint32_t m_size  = 0;
bool     m_write = true;
uint32_t m_tm    = 10000;
#define M_PAGE_SIZE 150

int m_page0[M_PAGE_SIZE];
int m_page_count0 = 0;


bool debug;
char s[100];


SButton b1(PIN_BUTTON ,100,2000,0,0,0); 
//UTFT myGLCD ( ILI9341_S5P, PIN_CS, PIN_RESET, PIN_DC );
Adafruit_ILI9341 display = Adafruit_ILI9341(PIN_CS, PIN_DC, PIN_RESET);

float u1=0.0,i1=0.0,p1=0.0,e1=0.0;
float p_max = 0, p_min = 99999999;
float u_avg=0.0,i_avg=0.0,p_avg=0.0;
int u_count=0,i_count=0,p_count=0;


void setup() {
 
// Последовательный порт для отладки
   DebugSerial.begin(115200);
   DebugSerial.println("PowerMonitor v 1.0");
// Считываем параметры из EEPROM   

   SPIFFS.begin();
   EC_begin();  
   EC_read();
// Инициализируем кнопку
   b1.begin();   
// Инициализируем дисплей
   display.begin();
   display.setRotation(1);
   display.fillScreen(ILI9341_BLACK);
   display.startWrite();
   display.setTextColor(ILI9341_YELLOW);
   display.setFont(&FreeMonoBold12pt7b);
   display.setCursor(10,16);
   display.print("PowerMeter v1.0");
   display.endWrite();
// Подключаемся к WiFi  
   WiFi_begin();
   delay(1000);

   display.startWrite();
   display.setFont(&FreeMono9pt7b);                           // устанавливаем маленький шрифт
   display.setTextColor(ILI9341_WHITE);
//   display.display();
// Старт внутреннего WEB-сервера
   if( isAP){ 
      sprintf(s,"AP: %s",NetConfig.ESP_NAME);
      display.setCursor(10,32);
      display.print(s);
      sprintf(s,"http://192.168.4.1"); 
      display.setCursor(10,48);
      display.print(s);
   }
   else {  
      sprintf(s,"Connect: %s",NetConfig.AP_SSID);
      display.setCursor(10,32);
      display.print(s);
      IPAddress my_ip = WiFi.localIP();
      sprintf(s,"http://%d.%d.%d.%d",my_ip[0],my_ip[1],my_ip[2],my_ip[3]); 
      display.setCursor(10,48);
      display.print(s);
   }
   display.endWrite();
   
   SetMFile();

   
// Запуск внутреннего WEB-сервера  
   HTTP_begin(); 
// Запуск модуля измерения ЭЭ   
   pzem.setAddress(ip);
// Инициализация UDP клиента для NTP
  NTP_begin();  
     
}

void loop() {
   ms = millis();
// Обработка кнопки    
   switch( b1.Loop() ){
      case  SB_CLICK :
         WriteMFile(true);
         break;
      case  SB_LONG_CLICK :
         if( m_write == true )m_write = false;
         else m_write = true;
         break;
   }
// Опрос показаний ЭЭ
   if( ms1 == 0 || ms < ms1 || (ms - ms1)>500 ){
       ms1    = ms;
       t_cur  = ms/1000;
       tm     = t_cur + t_correct;

       float u2,i2,p2,e2;
       for( int i=0; i<3; i++ ){
           u2 = pzem.voltage(ip);
           if( u2 >= 0 ){u1 = u2; u_avg+=u1; u_count++; break; }
       }
       for( int i=0; i<3; i++ ){
           i2 = pzem.current(ip);
           if( i2 >= 0 ){i1 = i2; i_avg+=i1; i_count++; break; }
       }
       for( int i=0; i<3; i++ ){
           p2 = pzem.power(ip);
           if( p2 >= 0 ){p1 = p2; p_avg+=p1; p_count++; break; }
       }
       for( int i=0; i<3; i++ ){
           e2 = pzem.energy(ip);
           if( e2 >= 0 ){e1 = e2; break; }
       }
       if( p_max < p1 )p_max = p1;
       if( p_min > p1 )p_min = p1;
   
       
       displayEE();

   }

   if( ms2 == 0 || ms < ms2 || (ms - ms2)>m_tm ){
       ms2 = ms;
       WriteMFile(false);
       displayGRAPH();
   }
   
   if( ms4 == 0 || ms < ms4 || (ms - ms4)>2000 ){
      ms4 = ms;
      if( m_page_count0 >= M_PAGE_SIZE-1 ){
        p_max = 0;p_min = 99999999;
// Сдвигаем график        
        for( int i=0; i<M_PAGE_SIZE-1; i++ ){
             m_page0[i] = m_page0[i+1];
             if( p_max < m_page0[i] )p_max = m_page0[i];
             if( p_min > m_page0[i] )p_min = m_page0[i];
        }
        m_page0[M_PAGE_SIZE-1] = p1;
        if( p_max < p1 )p_max = p1;
        if( p_min > p1 )p_min = p1;
      }
      else {
        m_page0[m_page_count0++] = p1;
      }
      
       
   }


// Опрос NTP сервера
   if( !isAP && ( ms3 == 0 || ms < ms3 || (ms - ms3)>NTP_TIMEOUT )){
       uint32_t t = GetNTP();
       if( t!=0 ){
          ms3 = ms;
          t_correct = t - t_cur;
       }
   }

 
   if( isAP && ms > AP_REBOOT_TM*1000 ){
       DebugSerial.printf("TIMEOUT. REBOOT. \n");
       ESP.reset();
   }
   HTTP_loop();

}


void displayEE(){
   display.startWrite();
   display.fillRect(0, 50, 320, 40,   ILI9341_BLACK);
   display.drawRect(0, 50, 159, 20,   ILI9341_WHITE);
   display.drawRect(160, 50, 159, 20, ILI9341_WHITE);
   display.drawRect(0, 70, 159, 20,   ILI9341_WHITE);
   display.drawRect(160, 70, 159, 20, ILI9341_WHITE);
  
   display.setFont(&FreeMonoBold12pt7b);
   display.setTextColor(ILI9341_GREEN);

   sprintf(s,"%d.%02d V",(int)u1,((int)(u1*100))%100);
   display.setCursor(2, 68);
   display.print(s);
   
   sprintf(s,"%d.%02d A",(int)i1,((int)(i1*100))%100);
   display.setCursor(162, 68);
   display.print(s);
    
   sprintf(s, "%d.%02d W", (int)p1 ,((int)(p1*100))%100);
   display.setCursor(2, 88);
   display.print(s);
       
   sprintf(s,"%d.%02d Wh",(int)e1,((int)(e1*100))%100);
   display.setCursor(162, 88);
   display.print(s);

// Отображение времени
   if( t_correct != 0 ){
//      tm    = t_cur + t_correct;
      sprintf(s, "%02d:%02d", (int)( tm/3600 )%24, (int)( tm/60 )%60);
      display.fillRect(240, 0, 80, 20,   ILI9341_BLACK);
      display.setCursor(240, 16);
      display.print(s);
    
   }
   
   display.endWrite();


   
}


void displayGRAPH(){
// Стираем область графика
   display.startWrite();
   display.fillRect(0, 120, 320, 120,   ILI9341_WHITE);


// Рисуем сетку
   for( int i=0; i<15; i++){
      int x = 18 + i*20;
      display.drawLine(x,138,x,238,ILI9341_BLACK);    
   }
   for( int i=0; i<5; i++){
      int y = 138 + i*20;
      display.drawLine(18,y,318,y,ILI9341_BLACK);    
   }
// Формируем график
   int n = 0;
   sprintf(s, "Pmax=%d.%02d Pmin=%d.%02d", (int)p_max ,((int)(p_max*100))%100, (int)p_min ,((int)(p_min*100))%100);
// Пишем минимум максимум
   display.setFont(&FreeMono9pt7b);                           // устанавливаем маленький шрифт
   display.setTextColor(ILI9341_RED);
   display.setCursor(30,135);
   display.print(s);
      
   for( int i=0; i<m_page_count0; i++){
      float yf = 1;
      if( p_max > 0 )yf = m_page0[i]*100/(p_max);
      int y = 238-(int)yf;
      int x  = 19 + n*2;
      n++;
      display.drawLine(x,y,x,237,ILI9341_RED);
   }

   display.endWrite();
}

void SetMFile(){
   int file_num = 0;
   Dir dir = SPIFFS.openDir("/");
   while (dir.next()) {    
      String fileName = dir.fileName();
      fileName.replace(".txt","");
      fileName.replace("/","");
      int n = fileName.toInt();
      if( n > file_num )file_num = n;
   }
   file_num++;
   sprintf(m_file,"/%08d.txt",file_num);
   m_count = 0;

}

void WriteMFile(bool flag){
    display.startWrite();
    display.fillRect(0, 90, 320, 20,   ILI9341_BLACK);
    if(m_write || flag ){
       if( u_count > 0 )u_avg/=u_count;
       if( i_count > 0 )i_avg/=i_count;
       if( p_count > 0 )p_avg/=p_count;
       File f;
       if( SPIFFS.exists(m_file) )f = SPIFFS.open(m_file, "a");
       else f = SPIFFS.open(m_file, "w");
       if( flag ){
          sprintf(s,"%ld;%d.%02d;%d.%02d;%d.%02d;%d.%02d;x\n",tm,
             (int)u1,((int)(u1*100))%100,
             (int)i1,((int)(i1*100))%100,
             (int)p1,((int)(p1*100))%100,
             (int)e1,((int)(e1*100))%100);
        
       }
       else {
          sprintf(s,"%ld;%d.%02d;%d.%02d;%d.%02d;%d.%02d;\n",tm,
             (int)u_avg,((int)(u_avg*100))%100,
             (int)i_avg,((int)(i_avg*100))%100,
             (int)p_avg,((int)(p_avg*100))%100,
             (int)e1,((int)(e1*100))%100);
       }
       
       f.print(s);
       m_size = f.size();
       f.close();
       u_avg=0; u_count=0;
       i_avg=0; i_count=0;
       p_avg=0; p_count=0;
       m_count++;
       display.setFont(&FreeMono9pt7b);                           // устанавливаем маленький шрифт
       display.setTextColor(ILI9341_WHITE);
       display.setCursor(1,105);
       sprintf(s,"%s(%d) %d rec",m_file,m_size,m_count);
       display.printf(s);
    }
    display.endWrite();
}

