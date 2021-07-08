/*
 * Metal detector 
 * Send value to server
 * with time
 * 
 */
 
// LIBRARY
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "RTClib.h"
#include <NTPClient.h>

// PROP
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// DECLARE
const int a = 35;//sensor       : sensor
const int b = 34;//defact light : defact
const int c = 32;//mode
const int s = 2;//light status
const int t = 4;//mode status

int sensor = 0; //sensor state
int defact = 0; //defact light state
int modes = 0; //mode state

bool objectDet = false; //just flag

// Replace with your network credentials
//const char* ssid     = "guest";
//const char* password = "syf-origin";
const char* ssid     = "MES";
const char* password = "00000000";
IPAddress staticIP(172,16,3,80); // IP the board
IPAddress gateway(172,16,3,254);
IPAddress subnet(255,255,0,0);

// REPLACE with your Domain name and URL path or IP address with path
//const char* serverName = "http://172.16.3.79:49908/api/Sensor/Sensor";  //MES
//const char* serverName = "http://172.16.165.79:49908/api/Sensor/Sensor";  //guest
const char* serverName = "http://172.16.1.7:80/MetalDetector/api/Sensor/Sensor";  //server

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "172.16.1.16", 25200);

void setup() {
  Serial.begin(115200);
  pinMode(a, INPUT);//PULL UP (idle is HIGH)
  pinMode(b, INPUT);//PULL DOWN (idle is LOW)
  pinMode(c, INPUT);//PULL DOWN (idle low)
  pinMode(s, OUTPUT); 
  pinMode(t, OUTPUT);

  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet);  //<--- comment if using dhcp
  while(WiFi.status() != WL_CONNECTED) { //led blinking if wifi no connect
    delay(150);
    digitalWrite(s, HIGH);
    delay(150);
    digitalWrite(s, LOW);
  }
  Serial.println(" OK 1 ");
  digitalWrite(s, HIGH);  //turn on led when wifi connected
  if (! rtc.begin()) {
    digitalWrite(s, LOW);
    while (1);            //if RTC not found, led off and device need to reset
  }
  timeClient.begin();
  timeClient.update();    //update ntp
  unsigned long epoc = timeClient.getEpochTime();
  String formattedDate = timeClient.getFormattedDate();
  int yr = formattedDate.substring(0, 4).toInt();
  int mn = formattedDate.substring(5, 7).toInt();
  int dt = formattedDate.substring(8, 10).toInt();
  Serial.println(" OK 2 ");
  Serial.println(epoc);
  //sync time rtc with ntp every device boot
  if (epoc > 1583894900)         //if ntp fail to get time, no sync rtc with ntp
  {
    rtc.adjust(DateTime(yr, mn, dt, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
  }
  delay(2000);            //wait 2s
  DateTime now = rtc.now();
  if (now.year() < 2020)  //if time from rtc error, device need to reset
  {
    digitalWrite(s, LOW);
    while (1);
  }
  Serial.println(" OK 3 ");
}

void loop() {

  while(WiFi.status() != WL_CONNECTED) { //led blinking if wifi no connect
    delay(150);
    digitalWrite(s, HIGH);
    delay(150);
    digitalWrite(s, LOW);
  }
  
  sensor = digitalRead(a);
  defact = digitalRead(b);
  modes = digitalRead(c);

  if (modes == HIGH)
  {
    digitalWrite(t, HIGH);
  }
  else
  {
    digitalWrite(t, LOW);
  }
  
  if (sensor==HIGH)        //No object detected (pull down) / HIGH for pull UP
  {
    //digitalWrite(t, LOW);
    Serial.println(" OK 4 NO");
    objectDet = true;
  }
  else if (sensor==LOW)    //Object Detected!
  {
    Serial.println(" OK 5 DET");
    //digitalWrite(t, HIGH);
    if (objectDet == true)
    {
      if (modes == LOW)
      {
        sendData(1);
      }
      Serial.println(" OK 6 PAS");   
    }
    objectDet = false;
  }
  if (defact == HIGH)  //Defact detect, hold 5 sec for blinking light aleart
  {
    if (modes == LOW)
    {
      sendData(2);
      delay(5100); 
    }
    Serial.println(" OK 7 DEF");
       
  }
  delay(170);
}

void sendData(int stat)
{
  String LocalIP = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." +
  WiFi.localIP()[3];
  DateTime now = rtc.now();
  int yyyy = now.year();
  int mm = now.month();
  int dd = now.day();
  int hh = now.hour();
  int mn = now.minute();
  int ss = now.second();
  String fullDateTime = String(yyyy) + "/" + String(mm) + "/" + String(dd) + " " + String(hh) + ":" + String(mn) + ":" + String(ss) + "";
  String justDate = String(yyyy) + "/" + String(mm) + "/" + String(dd) + "";
  if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      //String httpRequestData = "device_ip=172.16.3.80&device_mac=24:0A:C4:30:E8:90&detect_id=2&date_time=2020/3/9 18:1:40&date_only=2020/3/9;
      String httpRequestData = "device_ip=" + String(LocalIP) + "&device_mac=" + String(WiFi.macAddress())
                            + "&detect_id=" + String(stat) + "&date_time=" + String(fullDateTime) + "&date_only="
                            + String(justDate) + "";
      Serial.println(httpRequestData);
      int httpResponseCode = http.POST(httpRequestData);
      String payload = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(payload);
      http.end();
   }
   else {
   }
}
