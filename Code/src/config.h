#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPAsyncTCP.h>

#define MAC_ADDRESS_BYTE 6
#define CONFIG_SWITCH_PIN 13
#define CONFIG_LED_PIN 2
#define DEVICE_NAME "SwingFrogWakener"
#define FIRMWARE_VERSION "3.0.1"

//#define DEBUG

struct Config
{
  // station ssid
  String stSSID;
  // station password
  String stPASS;
  // wol target port
  uint16_t wolPort;
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
};

extern struct Config config;
extern bool configMode;
extern IPAddress broadcastIP;


void refreshState(StaticJsonDocument<512> &doc);

template <typename T>
size_t convertState(T dst);

StaticJsonDocument<512> convertState();

void loadConfig();

void saveConfig();