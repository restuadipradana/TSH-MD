/*
  LoRa Simple Gateway/Node Exemple

  This code uses InvertIQ function to create a simple Gateway/Node logic.

  Gateway - Sends messages with enableInvertIQ()
          - Receives messages with disableInvertIQ()

  Node    - Sends messages with disableInvertIQ()
          - Receives messages with enableInvertIQ()

  With this arrangement a Gateway never receive messages from another Gateway
  and a Node never receive message from another Node.
  Only Gateway to Node and vice versa.

  This code receives messages and sends a message every second.

  InvertIQ function basically invert the LoRa I and Q signals.

  See the Semtech datasheet, http://www.semtech.com/images/datasheet/sx1276.pdf
  for more on InvertIQ register 0x33.

  created 05 August 2018
  by Luiz H. Cassettari

  TSH;A01B;1;2021/06/15 04:40:39
*/

#include <SPI.h>              // include libraries
#include <LoRa.h>
#define LED_BUILTIN 2
const long frequency = 915E6; // LoRa Frequency

//--pin out definition
const int csPin = 5;          // LoRa radio chip select
const int resetPin = 14;      // LoRa radio reset
const int irqPin = 4;         // change for your board; must be a hardware interrupt pin
const int s = 35;//sensor       : sensor
const int d = 34;//defact light : defact
const int m = 32;//mode

//--value definition--
String msgdata;
boolean objDet = false;       //flag object detected on sensor
boolean metDet = false;       //flag metal detected
int lightBlink = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(s, INPUT);//PULL UP (idle is HIGH)
  pinMode(d, INPUT);//PULL DOWN (idle is LOW)
  pinMode(m, INPUT);//PULL DOWN (idle low)
  
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);             // if failed, do nothing
  }
  //LoRa.setSyncWord(0xA0);
  Serial.println("LoRa init succeeded.");
  Serial.println();
  Serial.println("LoRa Node");
  Serial.println("Only receive messages from gateways");
  Serial.println("Tx: invertIQ disable");
  Serial.println("Rx: invertIQ enable");
  Serial.println();

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  //delay(2000);
  //LoRa_sendMessage("TSH;A01A;1;1970/01/01 00:00:01");
}

void loop() {

  //while (digitalRead(m)); // calibrate mode
  
  if (digitalRead(s))     // no object detect
  {
    if (objDet == true)   //turn to no detected
    {
      objDet = false;
      if(!digitalRead(m))
      {
        //send data detected
        if (runEvery(450)) //pervent uC crashing while voltage not stable (sensor lebih dari 2x dalam 100 ms)
        {
          LoRa_sendMessage("TSH;A01A;1;1970/01/01 00:00:01");
          Serial.print("send ");
        }
        
      }
      Serial.println("detect 1");
    }
  }
  else                     // object detected
  {
    objDet = true;
  }

  if (digitalRead(d))      //light on (metyal deteted)
  {
    metDet = true;
  }
  else                      //light off
  {
    if (metDet)     //to turn off
    {
      metDet = false;
      if (runEvery2(550))
      {
        lightBlink++;
        Serial.print("Def light ");
        Serial.println(lightBlink);
        if (lightBlink == 3)
        {
          lightBlink = 0;
          if(!digitalRead(m))
          {
            //send data defact
            LoRa_sendMessage("TSH;A01A;2;1970/01/01 00:00:01");
            Serial.print("Send ");
          }
          Serial.println("detect 2");
        }
      }
    }
  }

//  if (runEvery(1500)) { // repeat every 1000 millis
//
//    String message = "TSH;";
//    message += "A01A";
//    message += "1";
//    message += "1970/01/01 00:00:01";
//
//    LoRa_sendMessage(message); // send a message
//    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
//    Serial.print("Send Message! ");
//    Serial.println(message);
//    ctr++;
//  }
//  if(Serial.available()){
//    msgdata = Serial.readStringUntil('\n');
//
//    Serial.println("Send Message! " + msgdata);
//    LoRa_sendMessage(msgdata);
//  }

}

void LoRa_rxMode(){
  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();               // normal mode
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
  //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  Serial.print("Node Receive: ");
  Serial.println(message);
}

void onTxDone() {
  //Serial.println("TxDone");
  LoRa_rxMode();
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
