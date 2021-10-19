#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <WiFiUDP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define AP_MODE 0
#define MAC_ADDRESS_BYTE 6
const String deviceName = "SwingFrogWakener";

struct Config
{
  // station ssid
  String stSSID;
  // station password
  String stPASS;
  // wol target port
  uint16_t wolPort;
  //mqtt enable flag
  boolean mqttEn;
  // mqtt uri
  String mqttHost;
  // mqtt port
  uint16_t mqttPort;
  // mqtt topic
  String mqttTopic;
} config;

WiFiUDP udp;
AsyncWebServer server(80);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
IPAddress broadcastIP(255, 255, 255, 255);
void loadConfig()
{
  File file = LittleFS.open("/config.json", "r+");
  if (!file)
    Serial.println("file open failed");
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println("Failed to read file, using default configuration");

  config.stSSID = doc["stSSID"] | "";
  config.stPASS = doc["stPASS"] | "";

  config.wolPort = doc["wolPort"] | 9;

  config.mqttEn = doc["mqttEn"] | false;
  config.mqttHost = doc["mqttHost"] | "";
  config.mqttPort = doc["mqttPort"] | 1883;
  config.mqttTopic = doc["mqttTopic"] | "";

  file.close();
}

void saveConfig()
{
  File file = LittleFS.open("/config.json", "w+");
  if (!file)
    Serial.println("file open failed");
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;
  doc["stSSID"] = config.stSSID;
  doc["stPASS"] = config.stPASS;

  doc["wolPort"] = config.wolPort;

  doc["mqttEn"] = config.mqttEn;
  doc["mqttHost"] = config.mqttHost;
  doc["mqttPort"] = config.mqttPort;
  doc["mqttTopic"] = config.mqttTopic;

  if (serializeJson(doc, file) == 0)
    Serial.println("Failed to write to file");
  file.close();
}

void initAP()
{
  IPAddress localIP(192, 168, 8, 1);
  WiFi.softAPConfig(localIP, localIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(deviceName, "");
}

void connectWiFi()
{
  WiFi.begin(config.stSSID, config.stPASS);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected, IP address: ");
  Serial.print(WiFi.localIP());
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found.");
}

void initWebServer()
{
  server.serveStatic("/", LittleFS, "/www").setDefaultFile("index.html");

  server.onNotFound(notFound);
  server.begin();
}

void wakeOnLan(IPAddress dstIp, byte *macAddress)
{
  byte sync[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  udp.beginPacket(dstIp, config.wolPort);
  udp.write(sync, sizeof(sync));
  for (char i = 0; i < 16; i++)
    udp.write(macAddress, sizeof(byte) * MAC_ADDRESS_BYTE);
  udp.endPacket();
}

void wakeByMacStr(String macStr)
{
  if (macStr.isEmpty())
  {
    return;
  }
  int colonIndex = macStr.indexOf(':');
  int minusIndex = macStr.indexOf('-');
  if (!(colonIndex ^ minusIndex))
  {
    mqtt.publish(config.mqttTopic.c_str(), "mac address format error.");
    return;
  }
  const char *separator = (colonIndex > 0 ? ":" : "-");

  char *str = new char[macStr.length() + 1];
  macStr.toCharArray(str, macStr.length() + 1, 0);
  byte mac[6];
  char *ptr;
  mac[0] = strtol(strtok(str, separator), &ptr, HEX);
  for (uint8_t i = 1; i < 6; i++)
    mac[i] = strtol(strtok(NULL, separator), &ptr, HEX);

  wakeOnLan(broadcastIP, mac);
  free(str);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  if (config.mqttTopic != topic)
  {
    Serial.println("recvice out of topic message");
    return;
  }
  String str((char *)payload);
  int separatorIndex = str.indexOf('#', 0);
  if (separatorIndex <= 0)
    return;
  String opt = str.substring(0, separatorIndex);
  if (opt == "wake")
  {
    String param = str.substring(separatorIndex + 1, length);
    wakeByMacStr(param);
  }
}

void mqttReconnect()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqtt.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt.publish(config.mqttTopic.c_str(), "👴🏻回来了");
      // ... and resubscribe
      mqtt.subscribe(config.mqttTopic.c_str(), 1);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void initMqtt()
{
  mqtt.setServer(config.mqttHost.c_str(), config.mqttPort);
  mqtt.setCallback(mqttCallback);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  LittleFS.begin();
  loadConfig();
  if (AP_MODE)
    initAP();
  else
    connectWiFi();

  initWebServer();

  if (config.mqttEn)
  {
    initMqtt();
  }
  /*   byte mac[] = {0xA8, 0xA1, 0x59, 0x46, 0xF7, 0xEF};
  wakeOnLan(IPAddress(255, 255, 255, 255), mac)); */
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (!mqtt.connected())
  {
    mqttReconnect();
  }
  mqtt.loop();
  delay(1000);
}