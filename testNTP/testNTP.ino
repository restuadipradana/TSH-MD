#include <NTPClient.h>
// change next line to use with another board/shield
//#include <ESP8266WiFi.h>
#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

const char *ssid     = "MES";
const char *password = "00000000";
String formattedDate;
IPAddress staticIP(172,16,3,80); // IP the board
IPAddress gateway(172,16,3,254);
IPAddress subnet(255,255,0,0);
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "172.16.1.16", 25200); //indonesia 25200

void setup(){
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();
}

void loop() {
  timeClient.update();
  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.println(timeClient.getFormattedDate());
  Serial.println(timeClient.getEpochTime());
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  formattedDate = timeClient.getFormattedDate();
  int yr = formattedDate.substring(0, 4).toInt();
  int mn = formattedDate.substring(5, 7).toInt();
  int dt = formattedDate.substring(8, 10).toInt();
  Serial.print(yr);
  Serial.print("-");
  Serial.print(mn);
  Serial.print("-");
  Serial.println(dt);
  String pp = ("1001");
  int xx = pp.substring(1).toInt();
  Serial.println(xx);

  delay(1000);
}
