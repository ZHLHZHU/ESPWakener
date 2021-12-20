#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <mqtt.h>
#include <config.h>
#include <web.h>

void initAP()
{
  IPAddress localIP(192, 168, 8, 1);
  WiFi.softAPConfig(localIP, localIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(DEVICE_NAME, "");
}

void connectWiFi()
{
  WiFi.begin(config.stSSID, config.stPASS);
  ESP8266WiFiClass::persistent(false);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(2000);
    Serial.print(".");
  }
  Serial.println("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void initOTA()
{
  ArduinoOTA.onStart(
      []()
      {
        String type;
        Serial.println(ArduinoOTA.getCommand());
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      });

  ArduinoOTA.onEnd(
      []()
      { Serial.println("\nEnd"); });

  ArduinoOTA.onProgress(
      [](unsigned int progress, unsigned int total)
      { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError(
      [](ota_error_t error)
      {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });
  ArduinoOTA.begin();
}

void setup()
{
  Serial.begin(115200);
  LittleFS.begin();
  loadConfig();
  // init gpio
  pinMode(CONFIG_SWITCH_PIN, INPUT_PULLUP);
  configMode = digitalRead(CONFIG_SWITCH_PIN) == LOW;
#ifdef DEBUG
  initOTA();
#endif
  if (configMode)
  {
    Serial.println("config mode");
    initAP();
    initWebServer();
    pinMode(CONFIG_LED_PIN, OUTPUT);
    initOTA();
  }
  else
  {
    connectWiFi();
    if (!config.mqttHost.isEmpty())
    {
      initMqtt();
    }
  }
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED && !config.mqttHost.isEmpty() && !mqtt.connected())
  {
    mqttReconnect();
  }
  mqtt.loop();
#ifdef DEBUG
  ArduinoOTA.handle();
#endif
  if (configMode)
  {
    ArduinoOTA.handle();
    digitalWrite(CONFIG_LED_PIN, !digitalRead(CONFIG_LED_PIN));
  }
  delay(1000);
}