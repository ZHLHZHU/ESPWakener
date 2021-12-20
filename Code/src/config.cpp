#include <config.h>

Config config;
bool configMode = false;
IPAddress broadcastIP(255, 255, 255, 255);
void refreshState(StaticJsonDocument<512> &doc)
{
  config.stSSID = doc["stSSID"] | "";
  config.stPASS = doc["stPASS"] | "";

  config.wolPort = doc["wolPort"] | 9;

  config.mqttHost = doc["mqttHost"] | "broker.emqx.io";
  config.mqttPort = doc["mqttPort"] | 1883;
  config.mqttUsername = doc["mqttUsername"] | "";
  config.mqttPass = doc["mqttPass"] | "";
  config.mqttTopic = doc["mqttTopic"] | "SwingFrogWakener";
}

template <typename T>
size_t convertState(T dst)
{
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;
  doc["stSSID"] = config.stSSID;
  doc["stPASS"] = config.stPASS;

  doc["wolPort"] = config.wolPort;

  doc["mqttHost"] = config.mqttHost;
  doc["mqttPort"] = config.mqttPort;
  doc["mqttUsername"] = config.mqttUsername;
  doc["mqttPass"] = config.mqttPass;
  doc["mqttTopic"] = config.mqttTopic;

  return serializeJson(doc, *dst);
}

StaticJsonDocument<512> convertState()
{
  StaticJsonDocument<512> doc;
  doc["stSSID"] = config.stSSID;
  doc["stPASS"] = config.stPASS;

  doc["wolPort"] = config.wolPort;

  doc["mqttHost"] = config.mqttHost;
  doc["mqttPort"] = config.mqttPort;
  doc["mqttUsername"] = config.mqttUsername;
  doc["mqttPass"] = config.mqttPass;
  doc["mqttTopic"] = config.mqttTopic;

  return doc;
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
  StaticJsonDocument<512> doc = convertState();
  serializeJson(doc, file);
  file.close();
  doc.clear();
}