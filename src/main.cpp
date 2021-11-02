#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <WiFiUDP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <SPIFFSEditor.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define DEBUG 1
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
  // mqtt user name
  String mqttUsername;
  // mqtt password
  String mqttPass;
  // mqtt topic
  String mqttTopic;
} config;

WiFiUDP udp;
AsyncWebServer server(80);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
IPAddress broadcastIP(255, 255, 255, 255);

void refreshState(StaticJsonDocument<512> &doc)
{
  config.stSSID = doc["stSSID"] | "";
  config.stPASS = doc["stPASS"] | "";

  config.wolPort = doc["wolPort"] | 9;

  config.mqttEn = doc["mqttEn"] | false;
  config.mqttHost = doc["mqttHost"] | "";
  config.mqttPort = doc["mqttPort"] | 1883;
  config.mqttUsername = doc["mqttUsername"] | "";
  config.mqttPass = doc["mqttPass"] | "";
  config.mqttTopic = doc["mqttTopic"] | "";
}

template <typename T>
size_t convertState(T *dst)
{
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;
  doc["stSSID"] = config.stSSID;
  doc["stPASS"] = config.stPASS;

  doc["wolPort"] = config.wolPort;

  doc["mqttEn"] = config.mqttEn;
  doc["mqttHost"] = config.mqttHost;
  doc["mqttPort"] = config.mqttPort;
  doc["mqttUsername"] = config.mqttUsername;
  doc["mqttPass"] = config.mqttPass;
  doc["mqttTopic"] = config.mqttTopic;

  return serializeJson(doc, *dst);
}

void loadConfig()
{
  File file = LittleFS.open("/config.json", "r+");
  if (!file)
    Serial.println("file open failed");
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println("Failed to read file, using default configuration");
  refreshState(doc);
  file.close();
}

void saveConfig()
{
  File file = LittleFS.open("/config.json", "w+");
  if (!file)
    Serial.println("file open failed");
  convertState(&file);
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
  WiFi.persistent(false);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(2000);
    Serial.print(".");
  }
  Serial.println("Connected, IP address: ");
  Serial.print(WiFi.localIP());
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found.");
}

void resConfig(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  convertState(response);
  request->send(response);
}

AsyncCallbackJsonWebHandler *updateConfigHandlerFactory()
{
  return new AsyncCallbackJsonWebHandler("/update-config", [](AsyncWebServerRequest *request, JsonVariant &json)
                                         {
                                           StaticJsonDocument<512> doc;
                                           if (json.is<JsonObject>())
                                           {
                                             doc = json.as<JsonObject>();
                                           }
                                           refreshState(doc);
                                           saveConfig();
                                           request->send(200, "text/plain", "ok");
                                         });
}

/**
 * OTA update handler
 */
void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  Serial.printf("handleOTAUpload: %s, %u, %u, %u\n", filename.c_str(), index, len, final);
  if (!index)
  {
    Serial.printf("Update: %s\n", filename.c_str());
    Update.runAsync(true);
    Update.setMD5(request->header("OTA-MD5").c_str());
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    Serial.printf("Max free Sketch Space: %u\n", maxSketchSpace);
    if (!Update.begin(maxSketchSpace))
    {
      //start with max available size
      Update.printError(Serial);
    }
  }
  Update.write(data, len);
  if (final)
  {
    if (Update.end(true))
    {
      Serial.printf("Update Success.\nRebooting...\n");
    }
    else
    {
      Update.printError(Serial);
    }
    ESP.restart();
  }
}

void initWebServer()
{
  server.serveStatic("/", LittleFS, "/www").setDefaultFile("index.html");
  server.on("/config", resConfig);
  //todo add ota interface
  server.on(
      "/ota", HTTP_POST, [](AsyncWebServerRequest *request)
      { request->send(200, "text/plain", "ok"); },
      [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        handleOTAUpload(request, filename, index, data, len, final);
      });
  server.addHandler(updateConfigHandlerFactory());
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
    // Attempt to connect
    if (mqtt.connect(deviceName.c_str()))
    {
      Serial.println("connected");
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
  pinMode(13, INPUT_PULLUP);
  if (DEBUG)
  {
    initAP();
    initWebServer();
  }
  connectWiFi();
  if (config.mqttEn)
  {
    initMqtt();
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED && config.mqttEn && !mqtt.connected())
  {
    mqttReconnect();
  }
  int pinState = digitalRead(13);
  if (pinState == LOW)
  {
    Serial.println("button pressed");
  }
  mqtt.loop();
  delay(1000);
}