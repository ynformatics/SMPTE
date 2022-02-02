#include <HTTP_Method.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <WiFi.h>
#include "time.h"
#include "frame.h"

const char* ntpServer = "pool.ntp.org";
const char * defaultTimezone = "GMT+0BST-1,M3.5.0/01:00:00,M10.5.0/02:00:00";

const int CONFIG_PIN = 27;
const int SIGNAL_PIN = 32;
const int EBU_PIN = 33;
bool normalBoot = false;
bool ebu = true;

WebServer  server(80); // default IP address: 192.168.4.1

struct EEPROMData{
  byte version;
  char ssid[32];
  char pass[50];
  char tz[50];
 };

EEPROMData eepromData;

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to get time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup()
{
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  pinMode(EBU_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  LoadEEProm();

  normalBoot = (digitalRead(CONFIG_PIN) == HIGH);
  ebu = (digitalRead(EBU_PIN) == HIGH);
  
  if(normalBoot)
  {
    //connect to WiFi
    Serial.printf("Connecting to %s ", eepromData.ssid);
    WiFi.begin(eepromData.ssid, eepromData.pass);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" CONNECTED");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    //init NTP 
    configTime(0,0, ntpServer);
    configTzTime(eepromData.tz, ntpServer);
    printLocalTime();

    //  //disconnect WiFi as it's no longer needed
    //  WiFi.disconnect(true);
    //  WiFi.mode(WIFI_OFF);
 
    Frame::begin(SIGNAL_PIN, ebu); 
  }
  else
  {
    Serial.println("Starting AP mode");
    WiFi.softAP("Leitch_1");

    server.on("/",webpage);
    server.on("/edit",response);
    server.begin();
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
  }
}

void loop()
{
  if(!normalBoot)
    server.handleClient();

  struct tm timeinfo;
  if(getLocalTime(&timeinfo))
  {
    Frame::setTime(&timeinfo);
  }
}

void webpage() 
{
    String body(  "<html><body>");
    body += "<form action='edit' method='post'>";
    body += "<p><label for='ssid'> SSID: </label> <input name='ssid' type='text' size = '32' value='"; body+=eepromData.ssid; body+="' /></p>";
    body += "<p><label for='pass'> Password: </label><input name='pass' type='password' size = '32' value='"; body+=eepromData.pass; body+="' /></p>";
    body += "<p><label for='tz'> Timezone: </label> <input name='tz' type='text' size = '50' value='"; body+=eepromData.tz; body+="' /></p>";
    body += "<p><button type='submit'>Update</button></p>";
    body += "</body></html>";
    
    server.send(200, "text/html", body);
}

void response()
{
  bool needSave = false;
  
  if(server.hasArg("ssid") && server.arg("ssid").length()>0 && server.arg("ssid").length()<32 && server.arg("ssid") != eepromData.ssid)
  { 
    server.arg("ssid").toCharArray(eepromData.ssid, 32);  
    needSave = true;
  } 
  
  if(server.hasArg("pass") && server.arg("pass").length()>0 && server.arg("pass").length()<50 && server.arg("pass") != eepromData.pass)
  { 
    server.arg("pass").toCharArray(eepromData.pass, 50);   
    needSave = true;
  } 

  if(server.hasArg("tz") && server.arg("tz").length()>0 && server.arg("tz").length()< 50 && server.arg("tz") != eepromData.tz)
  { 
    server.arg("tz").toCharArray(eepromData.tz, 50);  
    needSave = true;
  } 

  if(needSave)
    SaveEEProm();
    
  webpage();
}

void SaveEEProm()
{
  EEPROM.begin(sizeof(eepromData));
  EEPROM.put(0, eepromData);
  EEPROM.commit();
  EEPROM.end();
}

void LoadEEProm()
{
  EEPROM.begin(sizeof(eepromData));
  EEPROM.get(0, eepromData);
  EEPROM.end(); 
}
