/**
* Реализация HTTP сервера
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include "WC_HTTP.h"
#include "WC_EEPROM.h"
#include "logo.h"

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

bool isAP = false;
bool isConnect  = false;

String authPass = "";
String HTTP_User = "";
String WiFi_List;
int    UID       = -1;




/**
 * Старт WiFi
 */
void WiFi_begin(void){ 
// Определяем список сетей
  ListWiFi(WiFi_List);
// Подключаемся к WiFi

  isAP = false;
  if( ! ConnectWiFi()  ){
      DebugSerial.printf("Start AP %s\n",NetConfig.ESP_NAME);
      WiFi.mode(WIFI_STA);
      WiFi.softAP(NetConfig.ESP_NAME);
      isAP = true;
      DebugSerial.printf("Open http://192.168.4.1 in your browser\n");
      isConnect = false; 
 }
  else {
// Получаем статический IP если нужно  
      if( NetConfig.IP != 0 ){

         WiFi.config(NetConfig.IP,NetConfig.GW,NetConfig.MASK);
         DebugSerial.print("Open http://");
         DebugSerial.print(WiFi.localIP());
         DebugSerial.println(" in your browser");
      }
   }
// Запускаем MDNS
    MDNS.begin(NetConfig.ESP_NAME);
    DebugSerial.printf("Or by name: http://%s.local\n",NetConfig.ESP_NAME);
    isConnect = true; 
    

   
}

/**
 * Соединение с WiFi
 */
bool ConnectWiFi(void) {

  // Если WiFi не сконфигурирован
  if ( strcmp(NetConfig.AP_SSID, "none")==0 ) {
     DebugSerial.printf("WiFi is not config ...\n");
     return false;
  }

  WiFi.mode(WIFI_STA);

  // Пытаемся соединиться с точкой доступа
  DebugSerial.printf("\nConnecting to: %s/%s\n", NetConfig.AP_SSID, NetConfig.AP_PASS);
  WiFi.begin(NetConfig.AP_SSID, NetConfig.AP_PASS);
  delay(1000);

  // Максиммум N раз проверка соединения (12 секунд)
  for ( int j = 0; j < 15; j++ ) {
  if (WiFi.status() == WL_CONNECTED) {
      DebugSerial.print("\nWiFi connect: ");
      DebugSerial.print(WiFi.localIP());
      DebugSerial.print("/");
      DebugSerial.print(WiFi.subnetMask());
      DebugSerial.print("/");
      DebugSerial.println(WiFi.gatewayIP());
      return true;
    }
    delay(1000);
    DebugSerial.print(WiFi.status());
  }
  DebugSerial.printf("\nConnect WiFi failed ...\n");
  return false;
}

/**
 * Найти список WiFi сетей
 */
void ListWiFi(String &out){
  int n = WiFi.scanNetworks();
  if( n == 0 )out += "<p>Сетей WiFi не найдено";
  else {
     out = "<select name=\"AP_SSID\">\n";
     for (int i=0; i<n; i++){
       out += "    <option value=\"";
       out += WiFi.SSID(i);
       out += "\"";
       if( strcmp(WiFi.SSID(i).c_str(),NetConfig.AP_SSID) == 0 )out+=" selected";
       out += ">";
       out += WiFi.SSID(i);
       out += " [";
       out += WiFi.RSSI(i);
       out += "dB] ";
       out += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
       out += "</option>\n";
     }     
     out += "</select>\n";
  }
}

 

/**
 * Старт WEB сервера
 */
void HTTP_begin(void){
// Поднимаем WEB-сервер  
   server.on ( "/", HTTP_handleRoot );
   server.on ( "/config", HTTP_handleConfig );
   server.on ( "/default", HTTP_handleDefault );
   server.on ( "/login", HTTP_handleLogin );
   server.on ( "/logo", HTTP_handleLogo );
   server.on ( "/view", HTTP_handleView );

   server.on ( "/reboot", HTTP_handleReboot );

   server.onNotFound ( HTTP_handleDownload );
  //here the list of headers to be recorded
   const char * headerkeys[] = {"User-Agent","Cookie"} ;
   size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //ask server to track these headers
   server.collectHeaders(headerkeys, headerkeyssize );
   httpUpdater.setup(&server,"/update");


   
   server.begin();
   DebugSerial.printf( "HTTP server started ...\n" );
  
}


/**
 * Вывод в буфер одного поля формы
 */
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len, bool is_pass){
   char str[10];
   if( strlen( label ) > 0 ){
      out += "<td>";
      out += label;
      out += "</td>\n";
   }
   out += "<td><input name ='";
   out += name;
   out += "' value='";
   out += value;
   out += "' size=";
   sprintf(str,"%d",size);  
   out += str;
   out += " length=";    
   sprintf(str,"%d",len);  
   out += str;
   if( is_pass )out += " type='password'";
   out += "></td>\n";  
}

/**
 * Выаод заголовка файла HTML
 */
void HTTP_printHeader(String &out,const char *title, uint16_t refresh){
  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n";
  if( refresh ){
     char str[10];
     sprintf(str,"%d",refresh);
     out += "<meta http-equiv='refresh' content='";
     out +=str;
     out +="'/>\n"; 
  }
  out += "<title>";
  out += title;
  out += "</title>\n";
  out += "<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n</head>\n";
  out += "<body>\n";

  out += "<img src=/logo>\n";
  out += "<p><b>Device: ";
  out += NetConfig.ESP_NAME;

  if( UID < 0 )out +=" <a href=\"/login\">Authorisation</a>\n";
  else out +=" <a href=\"/login?DISCONNECT=YES\">Exit</a>\n";
  out += "</b></p>";
}   
 
/**
 * Выаод окнчания файла HTML
 */
void HTTP_printTail(String &out){
  out += "</body>\n</html>\n";
}

/**
 * Ввод имени и пароля
 */
void HTTP_handleLogin(){
  String msg;
// Считываем куки  
  if (server.hasHeader("Cookie")){   
//    DebugDebugSerial.print("Found cookie: ");
    String cookie = server.header("Cookie");
//    DebugDebugSerial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")){
    DebugSerial.println("Disconnect");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESP_PASS=\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  if ( server.hasArg("PASSWORD") ){
    String pass = server.arg("PASSWORD");
    
    if ( HTTP_checkAuth((char *)pass.c_str()) >=0 ){
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESP_PASS="+pass+"\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      DebugSerial.println("Login Success");
      return;
    }
  msg = "Bad password";
  DebugSerial.println("Login Failed");
  }
  String out = "";
  HTTP_printHeader(out,"Authorization");
  out += "<form action='/login' method='POST'>\
    <table border=0 width='600'>\
      <tr>\
        <td width='200'>Введите пароль:</td>\
        <td width='400'><input type='password' name='PASSWORD' placeholder='password' size='32' length='32'></td>\
      </tr>\
      <tr>\
        <td width='200'><input type='submit' name='SUBMIT' value='Ввод'></td>\
        <td width='400'>&nbsp</td>\
      </tr>\
    </table>\
    </form><b>" +msg +"</b><br>";
  HTTP_printTail(out);  
  server.send(200, "text/html", out);
}

/**
 * Обработчик событий WEB-сервера
 */
void HTTP_loop(void){
  server.handleClient();
}

/**
 * Перейти на страничку с авторизацией
 */
void HTTP_gotoLogin(){
  String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
  server.sendContent(header);
}
/**
 * Отображение логотипа
 */
void HTTP_handleLogo(void) {
  server.send_P(200, PSTR("image/png"), logo, sizeof(logo));
}
  


/*
 * Оработчик главной страницы сервера
 */
void HTTP_handleRoot(void) {
  char str[20];
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 
  
   if ( server.hasArg("mode") ){
       if( server.arg("mode") == "delete" ){
          if( server.arg("file") == "all" ){
             Dir dir = SPIFFS.openDir("/");
             while (dir.next()) {    
               if( dir.fileName().indexOf(".txt") > 0 )SPIFFS.remove(dir.fileName().c_str());
             }
             strcpy(m_file,"/00000001.txt");
          }
          else {
             SPIFFS.remove(server.arg("file").c_str());
            
          }   
          String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
          server.sendContent(header);
          return;
             
       }                    
    }
// Переключение режима измерения
   if ( server.hasArg("OK") ){
       m_tm = atoi( server.arg("TM").c_str());
       if( m_tm <=0 || m_tm > 300 )m_tm = 10;
       m_tm*=1000;
       String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
       server.sendContent(header);
       return;
   }
// Переключение режима измерения
   if ( server.hasArg("STOP") ){
       m_write = false;
       String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
       server.sendContent(header);
       return;
   }
// Переключение режима измерения
   if ( server.hasArg("START") ){
       m_write = true;
       String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
       server.sendContent(header);
       return;
   }


  
  String out = "";

  HTTP_printHeader(out,"PowerMeter_v1.0",10);
/*
  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n\
  <title>PowerMeter v1.0</title>\n\
  <style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n\
  </head>\n";
  out += "<body>\n";
*/  

   
   if( isAP ){
      out += "<p>Access point: ";
      out += NetConfig.ESP_NAME;
   }
   else {
      out += "<p>Connect to ";
      out += NetConfig.AP_SSID;
   }   
   out += "</p>\n";
   sprintf(str, "%02d:%02d", (int)( tm/3600 )%24, (int)( tm/60 )%60);
   out += "\n<h1>";
   out += str;
   out += "</h1>\n";
   

   out += "<p><a href=\"/config\">Set Config</a></p><br>\n";

   out += "<table border=1>\n<tr>\n<td><h1>";
   sprintf(str,"%d.%02d V",(int)u1,((int)(u1*100))%100);
   out += str;
   out += "</h1></td><td><h1>"; 
   sprintf(str,"%d.%02d A",(int)i1,((int)(i1*100))%100);
   out += str;
   out += "</h1></td>\n</tr>\n<tr>\n<td><h1>";    
   sprintf(str, "%d.%02d W", (int)p1 ,((int)(p1*100))%100);
   out += str;
   out += "</h1></td><td><h1>";       
   sprintf(str,"%d.%02d W/h",(int)e1,((int)(e1*100))%100); 
   out += str;
   out += "</h1></td>\n</tr>\n</table>\n";    



   out +="<form action='/' method='GET'>";
   out += "<p>Log timeout, s <input type='text' name='TM' value='";
   sprintf(str,"%ld",m_tm/1000);
   out += str;
   out += "'><input type='submit' name='OK' value='Apply'></p>\n";
   out += "<p>Logger: ";
   if( m_write )out += "<input type='submit' name='STOP' value='STOP'>";
   else out += "<input type='submit' name='START' value='START'>";
  
   out+= "</p>\n</form>\n";

   out += "<p>Log file: ";
   out += m_file;
   out += "</p>\n";
   out += "<ul>\n";
   Dir dir = SPIFFS.openDir("/");
   while (dir.next()) {    
      String fileName = dir.fileName();
      if( fileName.indexOf(".txt") > 0 ){
         out += "<li><a href=/view?file=";
         out += fileName;
         out += ">";
         out += fileName;
         out += " (";
         out += dir.fileSize();
         out += ") </a>  <a href=";
         out += fileName;
         out += ">[Download]</a> <a href=/?mode=delete&file=";
         out += fileName;
         out += ">[Delete]</a><br>\n";
     }
   }
   out += "</ul>\n<p><a href=\"/?mode=delete&file=all\">Delete all</a></p>";

   HTTP_printTail(out);
   


     
   server.send ( 200, "text/html", out );
}

 
/*
 * Оработчик страницы настройки сервера
 */
void HTTP_handleConfig(void) {
// Проверка прав администратора  
  char s[65];
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 

// Сохранение контроллера
  if ( server.hasArg("ESP_NAME") ){

     if( server.hasArg("ESP_NAME")     )strcpy(NetConfig.ESP_NAME,server.arg("ESP_NAME").c_str());
     if( server.hasArg("ESP_PASS")     )strcpy(NetConfig.ESP_PASS,server.arg("ESP_PASS").c_str());
     if( server.hasArg("AP_SSID")      )strcpy(NetConfig.AP_SSID,server.arg("AP_SSID").c_str());
     if( server.hasArg("AP_PASS")      )strcpy(NetConfig.AP_PASS,server.arg("AP_PASS").c_str());
     if( server.hasArg("IP1")          )NetConfig.IP[0] = atoi(server.arg("IP1").c_str());
     if( server.hasArg("IP2")          )NetConfig.IP[1] = atoi(server.arg("IP2").c_str());
     if( server.hasArg("IP3")          )NetConfig.IP[2] = atoi(server.arg("IP3").c_str());
     if( server.hasArg("IP4")          )NetConfig.IP[3] = atoi(server.arg("IP4").c_str());
     if( server.hasArg("MASK1")        )NetConfig.MASK[0] = atoi(server.arg("MASK1").c_str());
     if( server.hasArg("MASK2")        )NetConfig.MASK[1] = atoi(server.arg("MASK2").c_str());
     if( server.hasArg("MASK3")        )NetConfig.MASK[2] = atoi(server.arg("MASK3").c_str());
     if( server.hasArg("NASK4")        )NetConfig.MASK[3] = atoi(server.arg("MASK4").c_str());
     if( server.hasArg("GW1")          )NetConfig.GW[0] = atoi(server.arg("GW1").c_str());
     if( server.hasArg("GW2")          )NetConfig.GW[1] = atoi(server.arg("GW2").c_str());
     if( server.hasArg("GW3")          )NetConfig.GW[2] = atoi(server.arg("GW3").c_str());
     if( server.hasArg("GW4")          )NetConfig.GW[3] = atoi(server.arg("GW4").c_str());
     if( server.hasArg("WEB_PASS")   ){
         if( strcmp(server.arg("WEB_PASS").c_str(),"*") != 0 ){
             strcpy(NetConfig.WEB_PASS,server.arg("WEB_PASS").c_str());
         }
     }
     EC_save();
     
     String header = "HTTP/1.1 301 OK\r\nLocation: /config\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
  }

  String out = "";
  char str[10];
  HTTP_printHeader(out,"Config");
  out += "\
     <ul>\
     <li><a href=\"/\">Главная</a>\
     <li><a href=\"/default\">Set config default</a>\
     <li><a href=\"/update\">Firmware update</a>\
     <li><a href=\"/reboot\">Reset</a>\
     </ul>\n";

// Печатаем время в форму для корректировки

// Форма для настройки параметров
   out += "<h2>Config</h2>";
   out += "<h3>Access point params</h3>";
   out += "<form action='/config' method='POST'><table><tr>";
   HTTP_printInput(out,"AP Name:","ESP_NAME",NetConfig.ESP_NAME,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Password:","ESP_PASS",NetConfig.ESP_PASS,32,32,true);
   out += "</tr></table>";

   out += "<h3>WiFi params</h3>";
   out += "<table><tr>";
   out += "<td>WiFi Network</td><td>"+WiFi_List+"<td>";
//   HTTP_printInput(out,"Сеть WiFi:","AP_SSID",NetConfig.AP_SSID,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"WiFi Password: &nbsp;&nbsp;&nbsp;","AP_PASS",NetConfig.AP_PASS,32,32,true);
   out += "</tr></table><table><tr>";
   sprintf(str,"%d",NetConfig.IP[0]); 
   HTTP_printInput(out,"Static IP:","IP1",str,3,3);
   sprintf(str,"%d",NetConfig.IP[1]); 
   HTTP_printInput(out,".","IP2",str,3,3);
   sprintf(str,"%d",NetConfig.IP[2]); 
   HTTP_printInput(out,".","IP3",str,3,3);
   sprintf(str,"%d",NetConfig.IP[3]); 
   HTTP_printInput(out,".","IP4",str,3,3);
   out += "</tr><tr>";
   sprintf(str,"%d",NetConfig.MASK[0]); 
   HTTP_printInput(out,"Netmask","MASK1",str,3,3);
   sprintf(str,"%d",NetConfig.MASK[1]); 
   HTTP_printInput(out,".","MASK2",str,3,3);
   sprintf(str,"%d",NetConfig.MASK[2]); 
   HTTP_printInput(out,".","MASK3",str,3,3);
   sprintf(str,"%d",NetConfig.MASK[3]); 
   HTTP_printInput(out,".","MASK4",str,3,3);
   out += "</tr><tr>";
   sprintf(str,"%d",NetConfig.GW[0]); 
   HTTP_printInput(out,"Gateway","GW1",str,3,3);
   sprintf(str,"%d",NetConfig.GW[1]); 
   HTTP_printInput(out,".","GW2",str,3,3);
   sprintf(str,"%d",NetConfig.GW[2]); 
   HTTP_printInput(out,".","GW3",str,3,3);
   sprintf(str,"%d",NetConfig.GW[3]); 
   HTTP_printInput(out,".","GW4",str,3,3);
   out += "</tr><table>";

   out += "<h3>Controller auth</h3>";
   out += "<table><tr>";
   HTTP_printInput(out,"Password:","WEB_PASS","*",32,32,true);
   out += "</tr><table>";
   
   out +="<input type='submit' name='SUBMIT_CONF' value='Save'>"; 
   out += "</form></body></html>";
   server.send ( 200, "text/html", out );
  
}        



/*
 * Сброс настрое по умолчанию
 */
void HTTP_handleDefault(void) {
// Проверка прав администратора  
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 

  EC_default();
  HTTP_handleConfig();  
}



/**
 * Проверка авторизации
 */
int HTTP_isAuth(){
//  DebugDebugSerial.print("AUTH ");
  if (server.hasHeader("Cookie")){   
//    DebugDebugSerial.print("Found cookie: ");
    String cookie = server.header("Cookie");
//    DebugDebugSerial.print(cookie);
 
    if (cookie.indexOf("ESP_PASS=") != -1) {
      authPass = cookie.substring(cookie.indexOf("ESP_PASS=")+9);       
      return HTTP_checkAuth((char *)authPass.c_str());
    }
  }
  return -1;  
}


/**
 * Функция проверки пароля
 * возвращает 0 - админ, 1 - оператор, -1 - Не авторизован
 */
int  HTTP_checkAuth(char *pass){
   char s[32];
   strcpy(s,pass);
   if( strcmp(s,NetConfig.WEB_PASS) == 0 ){ 
       UID = 0;
       HTTP_User = "Admin";
   }
    else {
       UID = -1;
       HTTP_User = "User";
   }
   return UID;
}


/*
 * Перезагрузка часов
 */
void HTTP_handleReboot(void) {
// Проверка прав администратора  
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 


  String out = "";

  out = 
"<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='10;URL=/'>\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Controller reset... </h1>\
    </body>\
</html>";
   server.send ( 200, "text/html", out );
   ESP.reset();  
  
}

/*
 * Оработчик просмотра одного файла
 */
void HTTP_handleView(void) {
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 
   String file = server.arg("file");
   if( SPIFFS.exists(file) ){
      File f = SPIFFS.open(file, "r");
      size_t sent = server.streamFile(f, "text/plain");
      f.close();
   }
   else {
      server.send(404, "text/plain", "FileNotFound");
   }
}

/*
 * Оработчик скачивания одного файла
 */
void HTTP_handleDownload(void) {
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 
   String file = server.uri();
   if( SPIFFS.exists(file) ){
      File f = SPIFFS.open(file, "r");
      size_t sent = server.streamFile(f, "application/octet-stream");
      f.close();
   }
   else {
      server.send(404, "text/plain", "FileNotFound");
   }
}
