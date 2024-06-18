// #include <fmt/format.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <array>

#include <LoRa.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson>

constexpr uint8_t BROADCAST = 0xff;

constexpr uint8_t csPin = 15;          // LoRa radio chip select
constexpr uint8_t resetPin = 16;       // LoRa radio reset
constexpr uint8_t irqPin = 2;         // change for your board; must be a hardware interrupt pin

std::string_view ssid = "Kamchates";
std::string_view password = "12345678";

constexpr std::array<std::string_view, 5> NAMES = {
  "Медведь", 
  "Краб", 
  "Камчатка", 
  "Hello", 
  "Некто"
}; 

/*
INTERFACE ADDHL
0xAA - 0
0xBB - 1
0xCC - 2
0xDD - 3
0xFF - 4
*/
// std::unordered_map<uint8_t, std::string_view> ADRESSES{
//   {0xAA, NAMES[0]},
//   {0xBB, NAMES[1]},
//   {0xCC, NAMES[2],
//   {0xDD, NAMES[3]},
//   {0xFF, NAMES[4]}
// };



std::string_view serverIp = "192.168.4.3";  // IP адрес сервера, он статичен
constexpr uint16_t port = 5000;              // Порт для общения, пусть он совпадает у всех...


std::string outgoing;              // outgoing message

static uint8_t msgCount = 0;                   // count of outgoing messages
constexpr uint8_t localAddress = BROADCAST; // address of this device
constexpr uint8_t destination  = BROADCAST; // destination to send to

long lastSendTime = 0;        // last send time
constexpr uint32_t INTERVAL = 2000;          // interval between sends

std::string_view get_name(uint8_t sender){
  uint8_t base_addr = 0xAA;
  uint8_t transfer_index = (sender - base_addr) / 0x11;
  return NAMES[transfer_index];
}

void sendMessage(std::string outgoing);
void onReceive(size_t packetSize);
void send_post_request();
void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  Serial.println("LoRa init succeeded.");
  Serial.println("Я " + get_name(localAddress) + ", a это наш чат.");
  
  Serial.setTimeout(5);

  WiFi.begin(ssid.data(), password.data());
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

  std::string payload =  "{\"value\":\"" + outgoing + "\"}";
  uint8_t httpCode = http.POST(payload.c_str());
  
  if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
    Serial.println("POST запрос успешно отправлен");
  } else {
    Serial.print("Ошибка отправки POST запроса, код HTTP: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  }

  http.end();
}


void sendMessage(std::string outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address 
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing.c_str());                 // add payload
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

  std::string incoming = "";

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
  std::string_view sender_name = get_name(sender);

  // Serial.print(sender_name + ": " + incoming);
//  Serial.println("Sent to: 0x" + String(recipient, HEX));
//  Serial.println("Message ID: " + String(incomingMsgId));
//  Serial.println("Message length: " + String(incomingLength));
//  Serial.println("Message: " + incoming);
//  Serial.println("RSSI: " + String(LoRa.packetRssi()));
//  Serial.println("Snr: " + String(LoRa.packetSnr()));
  //Serial.println();
  outgoing = incoming;
}



