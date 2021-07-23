/*
 LORA - MQTT
 LORA GATEWAY

 LoRa data format:
 identifier;cell;status;time
 TSH;A01B;1;2021/06/15 04:40:39

 MQTT data format:
 {
  "c": "A01A",
  "g": "A01T",
  "d": 1,
  "t": "2021/06/14 13:12:10.568"
 }
 c = cell, g = gateway id, d = status detection, t = time detecty
 
 
*/
//---------Lib definition------------------------------
#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

//----------Pin Out config------------------------------
//const int csPin = 10;          // LoRa radio chip select
//const int resetPin = 9;        // LoRa radio reset
//const int irqPin = 2;          // change for your board; must be a hardware interrupt pin
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define LED_BUILTIN 2  //25 for heltec

//---------Configuration variable---------------------
// Replace the next variables with your SSID/Password combination
const char* ssid = "LA1ST";
const char* password = "tshorigin";
IPAddress staticIP(172,16,3,81); // IP the board
IPAddress gateway(172,16,3,254);
IPAddress subnet(255,255,0,0);

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "10.11.0.32"; // "172.16.0.11";

const long frequency = 915E6;  // LoRa Frequency

//--------initialize-----------------------------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
WiFiClient espClient;
PubSubClient client(espClient);


//-------local variable definition--------------------
#define MSG_BUFFER_SIZE  (128)
char msg[MSG_BUFFER_SIZE];
int value = 0;
bool sendIt = false;
String Data;
String d_ipAddr = "No connected";
String d_ssid = "No connected";
String i_cell = "";
int i_detect = 0;
String i_time = "";

//####################SETUP and LOOP#####################

void setup() {
  Serial.begin(9600);                   // initialize serial
  //while (!Serial);
  Data = " - ";
  pinMode(LED_BUILTIN, OUTPUT); //Send success, LED will bright 1 second
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  //LoRa.setSyncWord(0xF3);

  Serial.println("LoRa init succeeded.");
  Serial.println();
  Serial.println("LoRa Simple Gateway");
  Serial.println("Only receive messages from nodes");
  Serial.println("Tx: invertIQ enable");
  Serial.println("Rx: invertIQ disable");
  Serial.println();
  startOLED();
  WIFISetUp();
             
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();
}

void loop() {
  //if ((WiFi.status() != WL_CONNECTED) && runEvery2(3000)) {
  //  WIFISetUp();
  //}
  
  if (!client.connected()) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Connecting to MQTT");
    display.display();
    reconnect();
  }
  client.loop();

  

  if(sendIt)
  {
    if( i_cell != "" && i_detect != 0 && i_time != "" )
    {
      StaticJsonDocument<128> doc;
      doc["c"] = i_cell;
      doc["g"] = "A01T";
      doc["d"] = i_detect;
      doc["t"] = i_time;
      serializeJson(doc, msg);
      //snprintf (msg, MSG_BUFFER_SIZE, "");
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("md/abuilding", msg);
      sendIt = false;
      i_cell = "";
      i_detect = 0;
      i_time = "";
    }
  }
  
  if (runEvery(300)) 
  { 
    String message = "HeLoRa World! ";
    message += "I'm a Gateway! ";
    message += millis();
    //LoRa_sendMessage(message); // send a message
    //Serial.println("Send Message!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println(Data);
    display.setCursor(0,56);
    display.println(d_ipAddr);
    display.display();
  }
  
}

//#################END SETUP and LOOP##################

//------LORA----------
void LoRa_rxMode(){
  LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.enableInvertIQ();                // active invert I and Q signals
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
}

void onReceive(int packetSize) {
  String message = "";

  while (LoRa.available()) {
    message += (char)LoRa.read();
  }

  Serial.print("Gateway Receive: ");
  Serial.print(message);
  Serial.print(" with RSSI ");
  Serial.print(LoRa.packetRssi());
  Serial.print(" with size ");
  Serial.println(packetSize);
  if(message.substring(0,3) == "TSH")
  {
    Data = message;
    String message2 = "From TTGO ";
    message2 += Data;
    //LoRa_sendMessage(message2); // send a

    i_cell = getValue(Data,';',1);
    i_detect = getValue(Data,';',2).toInt();
    i_time = getValue(Data,';',3);
  
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    sendIt = true;
  }
}

void onTxDone() {
  //Serial.println("TxDone");
  LoRa_rxMode();
}


//------MQTT----------
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("md/recv");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



//---------STARTUP--------------

void WIFISetUp(void)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.begin(ssid,password);//fill in "Your WiFi SSID","Your Password"
  //WiFi.config(staticIP, gateway, subnet);  //<--- comment if using dhcp
  delay(100);

  byte count = 0;
  int ctt = 0;
  
  while(WiFi.status() != WL_CONNECTED )// && count < 70)
  {
    count ++;
    if (ctt == 4 ){ ctt = 0; }
    
    delay(100);
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Connecting to WiFi ");
    switch (ctt) {
    case 0:
      display.print("|");
      break;
    case 1:
      display.print("\\");
      break;
    case 2:
      display.print("-");
      break;
    case 3:
      display.print("/");
      break;
    default:
      // statements
      break;
    }
    display.display();
    ctt ++;
     
  }
  if(WiFi.status() == WL_CONNECTED)
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connected to ");
    display.println(WiFi.SSID());
    display.println(WiFi.localIP());
    display.display();
    d_ipAddr = IpAddress2String(WiFi.localIP());
    d_ssid = WiFi.SSID();
    delay(1500);
  }
  else
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Failed to connect..!!");
    display.display();
    //while(1);
  }
  delay(500);
}

void startOLED(){
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA GATEWAY TTGO");
  display.display();
  Serial.println(F("harusnya lcd udah mulai anjir"));
  delay(1000);
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

boolean runEvery2(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
