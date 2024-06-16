#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <string>
#include <LoRa.h>
#include <ESP8266HTTPClient.h>

const int csPin = 15;          // LoRa radio chip select
const int resetPin = 16;       // LoRa radio reset
const int irqPin = 2;         // change for your board; must be a hardware interrupt pin

const char* ssid = "Kamchates";
const char* password = "12345678";

WiFiUDP Udp;
const char* serverIp = "192.168.4.3";  // IP адрес сервера, он статичен
unsigned int port = 5000;              // Порт для общения, пусть он совпадает у всех...


String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xFF;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

String get_name(byte sender) {
  String sender_name = "";
  switch (sender) {
    case 0xAA:
      sender_name = "Медведь";
      break;
    case 0xBB:
      sender_name = "Краб";
      break;
    case 0xCC:
      sender_name = "Камчатка";
      break;
    case 0xDD:
      sender_name = "Hello";
      break;
    default:
      sender_name = "Некто";
  }
  return sender_name;
}
void sendMessage(String outgoing);
void onReceive(int packetSize);
void send_post_request();
void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);
  Serial.println("LoRa Duplex");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  Serial.println("Я " + get_name(localAddress) + ", a это наш чат.");
  
  Serial.setTimeout(5);

  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Подключаемся.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Клиент подключен к серверу! Присвоен IP: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  onReceive(LoRa.parsePacket());
  if(outgoing != ""){
    Serial.println();
    send_post_request();
    outgoing = "";
  }
}


void send_post_request() {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, "http://192.168.4.2:5000/temperature/create");
  http.addHeader("Content-Type", "application/json");

  String payload =  "{\"value\":\"" + outgoing + "\"}";
  int httpCode = http.POST(payload);
  
  if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
    Serial.println("POST запрос успешно отправлен");
  } else {
    Serial.print("Ошибка отправки POST запроса, код HTTP: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  }

  http.end();
}


void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address 
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    //Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
//  if (recipient != localAddress && recipient != 0xAA) {
//    Serial.println("This message is not for me.");
//    return;                             // skip rest of function
//  }

  // if message is for this device, or broadcast, print details:
  String sender_name = get_name(sender);

  Serial.print(sender_name + ": " + incoming);
//  Serial.println("Sent to: 0x" + String(recipient, HEX));
//  Serial.println("Message ID: " + String(incomingMsgId));
//  Serial.println("Message length: " + String(incomingLength));
//  Serial.println("Message: " + incoming);
//  Serial.println("RSSI: " + String(LoRa.packetRssi()));
//  Serial.println("Snr: " + String(LoRa.packetSnr()));
  //Serial.println();
  outgoing = incoming;
}


