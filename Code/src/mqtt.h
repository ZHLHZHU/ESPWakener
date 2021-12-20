#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <AsyncJson.h>
#include <wol.h>
#include <config.h>
#include <validation.h>


extern WiFiClient wifiClient;
extern PubSubClient mqtt;

bool mqttConnect();

void mqttReconnect();

void initMqtt();

void resDeviceInfo(const char *action);
